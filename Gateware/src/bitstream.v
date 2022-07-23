// Default Bitstream design for ICE-V Wireless FPGA
// 03-14-19 E. Brombaugh

`default_nettype none

`define DIV 14'd9999

module bitstream(
	// 12MHz external osc
	input clk_12MHz,
	
	// riscv diags
	output clk24,
	output reset24,
	output diag,
	output mbx_rinc,
	output mbx_winc,
	
	// RISCV serial
	input rx,
	output tx,
	
	// RISCV SPI
	inout	spi0_mosi,		// SPI core 0
			spi0_miso,
			spi0_sclk,
			spi0_cs0,
	output	spi0_nwp,
			spi0_nhld,
			
	// RGB output
    output RGB0, RGB1, RGB2, // RGB LED outs
	
	// SPI slave port
	input SPI_CSL,
	input SPI_MOSI,
	output SPI_MISO,
	input SPI_SCLK
);
	// This should be unique so firmware knows who it's talking to
	parameter DESIGN_ID = 32'hB00F0001;

	//------------------------------
	// Clock PLL
	//------------------------------
	// Fin=12, FoutA=24, FoutB=48
	wire clk, clk24, pll_lock;
	SB_PLL40_2F_PAD #(
		.DIVR(4'b0000),
		.DIVF(7'b0111111),	// 24/48
		.DIVQ(3'b100),
		.FILTER_RANGE(3'b001),
		.FEEDBACK_PATH("SIMPLE"),
		.DELAY_ADJUSTMENT_MODE_FEEDBACK("FIXED"),
		.FDA_FEEDBACK(4'b0000),
		.DELAY_ADJUSTMENT_MODE_RELATIVE("FIXED"),
		.FDA_RELATIVE(4'b0000),
		.SHIFTREG_DIV_MODE(2'b00),
		.PLLOUT_SELECT_PORTA("GENCLK_HALF"),
		.PLLOUT_SELECT_PORTB("GENCLK"),
		.ENABLE_ICEGATE_PORTA(1'b0),
		.ENABLE_ICEGATE_PORTB(1'b0)
	)
	pll_inst (
		.PACKAGEPIN(clk_12MHz),
		.PLLOUTCOREA(),
		.PLLOUTGLOBALA(clk24),
		.PLLOUTCOREB(),
		.PLLOUTGLOBALB(clk),
		.EXTFEEDBACK(),
		.DYNAMICDELAY(8'h00),
		.RESETB(1'b1),
		.BYPASS(1'b0),
		.LATCHINPUTVALUE(),
		.LOCK(pll_lock),
		.SDI(),
		.SDO(),
		.SCLK()
	);
	
	//------------------------------
	// reset generator waits > 10us
	//------------------------------
	reg [9:0] reset_cnt;
	reg reset, reset24;    
	always @(posedge clk or negedge pll_lock)
	begin
		if(!pll_lock)
		begin
			reset_cnt <= 10'h000;
			reset <= 1'b1;
		end
		else
		begin
			if(reset_cnt != 10'h3ff)
			begin
				reset_cnt <= reset_cnt + 10'h001;
				reset <= 1'b1;
			end
			else
				reset <= 1'b0;
		end
	end
	
	always @(posedge clk24 or negedge pll_lock)
		if(!pll_lock)
			reset24 <= 1'b1;
		else
			reset24 <= reset;
	
	//------------------------------
	// Internal SPI slave port
	//------------------------------
	wire [31:0] wdat;
	reg [31:0] rdat;
	wire [6:0] addr;
	wire re, we;
	spi_slave
		uspi(.clk(clk), .reset(reset),
			.spiclk(SPI_SCLK), .spimosi(SPI_MOSI),
			.spimiso(SPI_MISO), .spicsl(SPI_CSL),
			.we(we), .re(re), .wdat(wdat), .addr(addr), .rdat(rdat));
	
	//------------------------------
	// Writeable registers
	//------------------------------
	reg [7:0] per;
	reg [31:0] gpio_torisc;
	always @(posedge clk)
	begin
		if(reset)
		begin
			per <= 8'h4;
		end
		else if(we)
		begin
			case(addr)
				7'h01: per <= wdat;
				7'h02: gpio_torisc <= wdat;
			endcase
		end
	end
	
	//------------------------------
	// readback
	//------------------------------
	wire [31:0] gpio_fromrisc;
	wire [7:0] mbx_odat;
	wire mbx_oval;
	wire [11:0] mbx_diag;
	always @(*)
	begin
		case(addr)
			7'h00: rdat = DESIGN_ID;
			7'h01: rdat = per;
			7'h02: rdat = gpio_torisc;
			7'h03: rdat = gpio_fromrisc;
			7'h04: rdat = mbx_odat;
			7'h05: rdat = mbx_oval;
			7'h06: rdat = mbx_diag;
			default: rdat = 32'd0;
		endcase
	end
	
	//------------------------------
	// mailbox read pulse & sync
	//------------------------------
	reg [1:0] mbx_ord;
	always @(posedge clk)
	begin
		if(reset)
		begin
			mbx_ord <= 2'b00;
		end
		else
		begin
			mbx_ord <= {mbx_ord[0], (re & (addr == 7'h04))};
		end
	end
	assign mbx_rinc = mbx_diag[0];
	assign mbx_winc = mbx_diag[1];
	
	// RISC-V CPU based serial I/O
	wire [7:0] red, grn, blu;
	system u_riscv(
		.clk24(clk24),
		.reset(reset24),
		.RX(rx),
		.TX(tx),
		.spi0_mosi(spi0_mosi),
		.spi0_miso(spi0_miso),
		.spi0_sclk(spi0_sclk),
		.spi0_cs0(spi0_cs0),
		.gp_in0(32'h0),
		.gp_in1(gpio_torisc),
		.gp_in2(32'h0),
		.gp_in3(32'h0),
		.gp_out0({red,grn,blu}),
		.gp_out1(gpio_fromrisc),
		.gp_out2(),
		.gp_out3(),
		.ext_clk(clk),
		.ext_reset(reset),
		.mbx_odat(mbx_odat),
		.mbx_oval(mbx_oval),
		.mbx_ord(mbx_ord[1]),
		.diag(diag),
		.mbx_diag(mbx_diag)
	);
	assign spi0_nwp = 1'b1;
	assign spi0_nhld = 1'b1;

	//------------------------------
	// PWM dimming for the RGB DRV 
	//------------------------------
	reg [11:0] pwm_cnt;
	reg led_ena, r_pwm, g_pwm, b_pwm;
	always @(posedge clk)
		if(reset)
		begin
			pwm_cnt <= 8'd0;
			led_ena <= 1'b1;
		end
		else
		begin
			pwm_cnt <= pwm_cnt + 1;
			led_ena <= pwm_cnt[3:0] <= per;
			r_pwm <= pwm_cnt[11:4] < red;
			g_pwm <= pwm_cnt[11:4] < grn;
			b_pwm <= pwm_cnt[11:4] < blu;
		end
	
	//------------------------------
	// Instantiate RGB DRV 
	//------------------------------
	SB_RGBA_DRV #(
		.CURRENT_MODE("0b1"),
		.RGB0_CURRENT("0b000001"),
		.RGB1_CURRENT("0b000001"),
		.RGB2_CURRENT("0b000001")
	) RGBA_DRIVER (
		.CURREN(1'b1),
		.RGBLEDEN(led_ena),
		.RGB0PWM(r_pwm),
		.RGB1PWM(g_pwm),
		.RGB2PWM(b_pwm),
		.RGB0(RGB0),
		.RGB1(RGB1),
		.RGB2(RGB2)
	);
	
	//assign LEDR = state[0];
endmodule
