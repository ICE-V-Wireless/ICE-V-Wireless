/*
 * system.v - top-level system for picorv32 in up5k_osc
 * 04-26-21 E. Brombaugh
 */

`default_nettype none

module system(
	// clock, reset
	input	clk24,
			reset,
	
	// serial
	input	RX,
	output	TX,
	
	// GPIO
	input [31:0] gp_in0, gp_in1, gp_in2, gp_in3,
	output reg [31:0] gp_out0, gp_out1, gp_out2, gp_out3,
	
	// Wishbone
	inout	spi0_mosi,		// SPI core 0
			spi0_miso,
			spi0_sclk,
			spi0_cs0,
		
	inout	i2c0_sda,		// I2C core 0
			i2c0_scl,
	
	// Mailbox
	input ext_clk,			// output domain clock
	input ext_reset,		// output domain reset
	output [7:0] mbx_odat,	// output data bus
	output mbx_oval,		// output data valid
	input mbx_ord,			// output data read
	
	// Diagnostics
	output diag,
	output [11:0] mbx_diag
);
	// CPU
	wire        mem_valid;
	wire        mem_instr;
	wire        mem_ready;
	wire [31:0] mem_addr;
	reg  [31:0] mem_rdata;
	wire [31:0] mem_wdata;
	wire [ 3:0] mem_wstrb;
	picorv32 #(
		.PROGADDR_RESET(32'h 0000_0000),	// start or ROM
		.STACKADDR(32'h 1001_0000),			// end of SPRAM
		.BARREL_SHIFTER(0),
		.COMPRESSED_ISA(0),
		.ENABLE_COUNTERS(0),
		.ENABLE_MUL(0),
		.ENABLE_DIV(0),
		.ENABLE_IRQ(0),
		.ENABLE_IRQ_QREGS(0),
		.CATCH_MISALIGN(0),
		.CATCH_ILLINSN(0)
	) cpu_I (
		.clk       (clk24),
		.resetn    (~reset),
		.mem_valid (mem_valid),
		.mem_instr (mem_instr),
		.mem_ready (mem_ready),
		.mem_addr  (mem_addr),
		.mem_wdata (mem_wdata),
		.mem_wstrb (mem_wstrb),
		.mem_rdata (mem_rdata)
	);
	
	// Address decode
	wire rom_sel = (mem_addr[31:28]==4'h0)&mem_valid ? 1'b1 : 1'b0;
	wire ram_sel = (mem_addr[31:28]==4'h1)&mem_valid ? 1'b1 : 1'b0;
	wire gio_sel = (mem_addr[31:28]==4'h2)&mem_valid ? 1'b1 : 1'b0;
	wire ser_sel = (mem_addr[31:28]==4'h3)&mem_valid ? 1'b1 : 1'b0;
	wire wbb_sel = (mem_addr[31:28]==4'h4)&mem_valid ? 1'b1 : 1'b0;
	wire cnt_sel = (mem_addr[31:28]==4'h5)&mem_valid ? 1'b1 : 1'b0;
	wire mbx_sel = (mem_addr[31:28]==4'h6)&mem_valid ? 1'b1 : 1'b0;
	
	// 2k x 32 ROM
	reg [31:0] rom[2047:0], rom_do;
	initial
        $readmemh("rom.hex",rom);		
	always @(posedge clk24)
		rom_do <= rom[mem_addr[12:2]];
	
	// RAM, byte addressable
	wire [31:0] ram_do;
`define RAM64K
`ifdef RAM64K
	spram_16kx32 uram(
		.clk(clk24),
		.sel(ram_sel),
		.we(mem_wstrb),
		.addr(mem_addr[15:0]),
		.wdat(mem_wdata),
		.rdat(ram_do)
	);
`else
	bram_512x32 uram(
		.clk(clk24),
		.sel(ram_sel),
		.we(mem_wstrb),
		.addr(mem_addr[10:0]),
		.wdat(mem_wdata),
		.rdat(ram_do)
	);
`endif
	
	// GPIO Output
	always @(posedge clk24)
		if(gio_sel)
			case(mem_addr[3:2])
				2'b00:
				begin
					if(mem_wstrb[0])
						gp_out0[7:0] <= mem_wdata[7:0];
					if(mem_wstrb[1])
						gp_out0[15:8] <= mem_wdata[15:8];
					if(mem_wstrb[2])
						gp_out0[23:16] <= mem_wdata[23:16];
					if(mem_wstrb[3])
						gp_out0[31:24] <= mem_wdata[31:24];
				end
				2'b01:
				begin
					if(mem_wstrb[0])
						gp_out1[7:0] <= mem_wdata[7:0];
					if(mem_wstrb[1])
						gp_out1[15:8] <= mem_wdata[15:8];
					if(mem_wstrb[2])
						gp_out1[23:16] <= mem_wdata[23:16];
					if(mem_wstrb[3])
						gp_out1[31:24] <= mem_wdata[31:24];
				end
				2'b10:
				begin
					if(mem_wstrb[0])
						gp_out2[7:0] <= mem_wdata[7:0];
					if(mem_wstrb[1])
						gp_out2[15:8] <= mem_wdata[15:8];
					if(mem_wstrb[2])
						gp_out2[23:16] <= mem_wdata[23:16];
					if(mem_wstrb[3])
						gp_out2[31:24] <= mem_wdata[31:24];
				end
				2'b11:
				begin
					if(mem_wstrb[0])
						gp_out3[7:0] <= mem_wdata[7:0];
					if(mem_wstrb[1])
						gp_out3[15:8] <= mem_wdata[15:8];
					if(mem_wstrb[2])
						gp_out3[23:16] <= mem_wdata[23:16];
					if(mem_wstrb[3])
						gp_out3[31:24] <= mem_wdata[31:24];
				end
			endcase
	
	// GPIO input
	reg [31:0] gio_do;
	always @(posedge clk24)
		if(gio_sel)
			case(mem_addr[3:2])
				2'b00: gio_do <= gp_in0;
				2'b01: gio_do <= gp_in1;
				2'b10: gio_do <= gp_in2;
				2'b11: gio_do <= gp_in3;
			endcase
	
	// Serial
	wire [7:0] ser_do;
	acia uacia(
		.clk(clk24),			// system clock
		.rst(reset),			// system reset
		.cs(ser_sel),			// chip select
		.we(mem_wstrb[0]),		// write enable
		.rs(mem_addr[2]),		// address
		.rx(RX),				// serial receive
		.din(mem_wdata[7:0]),	// data bus input
		.dout(ser_do),			// data bus output
		.tx(TX),				// serial transmit
		.irq()					// interrupt request
	);
	
	// 256B Wishbone bus master and SB IP cores @ F100-F1FF
	wire [7:0] wbb_do;
	wire wbb_rdy;
	wb_bus uwbb(
		.clk(clk24),			// system clock
		.rst(reset),			// system reset
		.cs(wbb_sel),			// chip select
		.we(mem_wstrb[0]),		// write enable
		.addr(mem_addr[9:2]),	// address
		.din(mem_wdata[7:0]),	// data bus input
		.dout(wbb_do),			// data bus output
		.rdy(wbb_rdy),			// bus ready
		.spi0_mosi(spi0_mosi),	// spi core 0 mosi
		.spi0_miso(spi0_miso),	// spi core 0 miso
		.spi0_sclk(spi0_sclk),	// spi core 0 sclk
		.spi0_cs0(spi0_cs0),	// spi core 0 cs
		.i2c0_sda(i2c0_sda),	// i2c core 0 data
		.i2c0_scl(i2c0_scl)		// i2c core 0 clk
	);
	
	// Resettable clock counter
	reg [31:0] cnt;
	always @(posedge clk24)
		if(cnt_sel & |mem_wstrb)
		begin
			if(mem_wstrb[0])
				cnt[7:0] <= mem_wdata[7:0];
			if(mem_wstrb[1])
				cnt[15:8] <= mem_wdata[15:8];
			if(mem_wstrb[2])
				cnt[23:16] <= mem_wdata[23:16];
			if(mem_wstrb[3])
				cnt[31:24] <= mem_wdata[31:24];
		end
		else
			cnt <= cnt + 32'd1;
	
	// Mailbox
	wire [7:0] mbx_do;
	mailbox umbx(
		.clk(clk24),			// system clock
		.reset(reset),			// system reset
		.cs(mbx_sel),			// chip select
		.we(mem_wstrb[0]),		// write enable for 8 lsbits
		.addr(mem_addr[2]),		// address
		.din(mem_wdata[7:0]),	// data bus input
		.dout(mbx_do),			// data bus output
		.mbx_oclk(ext_clk),		// output domain clock
		.mbx_orst(ext_reset),	// output domain reset
		.mbx_odat(mbx_odat),	// output data bus
		.mbx_ompt(mbx_oval),	// output data valid
		.mbx_ord(mbx_ord),		// output data read
		.mbx_diag(mbx_diag)		// mailbox diags
	);
		
	// Read Mux
	always @(*)
		casex({mbx_sel,cnt_sel,wbb_sel,ser_sel,gio_sel,ram_sel,rom_sel})
			7'b0000001: mem_rdata = rom_do;
			7'b000001x: mem_rdata = ram_do;
			7'b00001xx: mem_rdata = gio_do;
			7'b0001xxx: mem_rdata = {{24{1'b0}},ser_do};
			7'b001xxxx: mem_rdata = {{24{1'b0}},wbb_do};
			7'b01xxxxx: mem_rdata = cnt;
			7'b1xxxxxx: mem_rdata = mbx_do;
			default: mem_rdata = 32'd0;
		endcase
	
	// ready flag
	reg mem_rdy;
	always @(posedge clk24)
		if(reset)
			mem_rdy <= 1'b0;
		else
			mem_rdy <= (mbx_sel|cnt_sel|ser_sel|gio_sel|ram_sel|rom_sel) & ~mem_rdy;
	assign mem_ready = wbb_rdy | mem_rdy;
		
	// hook up diag
	assign diag = ram_sel;
endmodule

