// lp4k_bob - lattice ice5lp4k LED blinky for BOB with SPI control port
// this version is for icestorm and has been bodged a bit due to current
// lack of support for th RGB driver
// 04-09-22 E. Brombaugh

`define DIV 14'd16383

module lp4k_bob(	
	output reg pin_41,
	
	// SPI slave port
	input SPI_CSL,
	input SPI_MOSI,
	output SPI_MISO,
	input SPI_SCLK
);
	// This should be unique so firmware knows who it's talking to
	parameter DESIGN_ID = 32'hB00C0000;

	//------------------------------
	// Instantiate HF Osc with div 1
	//------------------------------
	wire clk;
	SB_HFOSC #(.CLKHF_DIV("0b00")) OSCInst0 (
		.CLKHFEN(1'b1),
		.CLKHFPU(1'b1),
		.CLKHF(clk)
	);
	
	//------------------------------
	// Instantiate LF Osc
	//------------------------------
	wire CLKLF;
	SB_LFOSC OSCInst1 (
		.CLKLFEN(1'b1),
		.CLKLFPU(1'b1),
		.CLKLF(CLKLF)
	);
	
	//----------------------------------------------------------------------
	// reset generator - 4 clocks of high-true reset after coming out of cfg
	//----------------------------------------------------------------------
	reg [3:0] reset_pipe = 4'h0;
	reg reset = 1'b0;
	always @(posedge clk)
	begin
		reset <= ~(&reset_pipe);
		reset_pipe <= {reset_pipe[2:0],1'b1};
	end
	
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
	reg [13:0] per;
	always @(posedge clk)
	begin
		if(reset)
		begin
			per <= 14'h3fff;
		end
		else if(we)
		begin
			case(addr)
				7'h01: per <= wdat;
			endcase
		end
	end
	
	//------------------------------
	// readback
	//------------------------------
	wire [31:0] readbus;
	always @(*)
	begin
		case(addr)
			7'h00: rdat = DESIGN_ID;
			7'h01: rdat = per;
			default: rdat = 32'd0;
		endcase
	end
	
	//------------------------------
	// Divide the clock
	//------------------------------
	reg [13:0] clkdiv;
	reg onepps;
	always @(posedge CLKLF)
	begin		
		if(clkdiv == 14'd0)
		begin
			onepps <= 1'b1;
			clkdiv <= `DIV;
		end
		else
		begin
			onepps <= 1'b0;
			clkdiv <= clkdiv - per;
		end
	end
	
	//------------------------------
	// LED signals
	//------------------------------
	reg [2:0] state;
	always @(posedge CLKLF)
	begin
		if(onepps)
			state <= state + 3'd1;
	end
	
	// simple blink
    assign pin_26 = state[0];
	
	// breathing
	reg [4:0] thresh;
	always @(posedge CLKLF)
	begin
		if(state[0])
			thresh <= (``DIV-clkdiv)>>9;
		else
			thresh <= clkdiv>>9;
		
		pin_41 <= (clkdiv[4:0] >= thresh);		
	end
endmodule
