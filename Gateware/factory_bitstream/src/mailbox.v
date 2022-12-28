/*
 * mailbox.v - Async FIFO based mailbox between RISC-V system and external
 * 06-04-22 E. Brombaugh
 */

`default_nettype none

module mailbox(
	// clock, reset
	input	clk,		// RISC-V domain clock
			reset,		// RISC-V domain reset
			
	// RISC bus interface
	input 	cs,			// chip select
			we,			// write enable for 8 lsbits
			addr,		// address
	input [7:0]	din,	// data bus input
	output reg [7:0] dout,	// data bus output
	
	// external interface
	input	mbx_oclk,		// output domain clock
			mbx_orst,		// output domain reset
	output [7:0] mbx_odat,	// output data bus
	output	mbx_ompt,		// output data valid
	input	mbx_ord,		// output data read
	
	// diags
	output [11:0] mbx_diag
);
	// rising edge detect on winc
	wire write = cs&we&(addr==1'b1);
	reg dwrite;
	always @(posedge clk)
		dwrite <= write;
	wire winc = write & !dwrite;
	
	// rising edge detect on rinc
	wire read = mbx_ord;
	reg dread;
	always @(posedge mbx_oclk)
		dread <= read;
	wire rinc = read & !dread;
	
	// Output FIFO
	wire wfull;
	wire [4:0] waddr, raddr;
	fifo1 #(.DSIZE(8), .ASIZE(4)) uofifo(
		.rdata(mbx_odat),
		.wfull(wfull),
		.rempty(mbx_ompt),
		.wdata(din),
		.winc(winc),
		.wclk(clk),
		.wrst_n(!reset),
		.rinc(rinc),
		.rclk(mbx_oclk),
		.rrst_n(!mbx_orst),
		.waddr(waddr),
		.raddr(raddr)
	);
	
	// Input FIFO (placeholder)
	wire rempty = 1'b1;
	wire [7:0] rx_dat = 8'h00;

	// build status word
	wire [7:0] status = 
	{
		6'b000000,			// bits 7-2 unused
		wfull,				// bit 1 = tx full
		rempty				// bit 0 = rx empty
	};

	// load dout with either status or rx data
	always @(posedge clk)
	begin
		if(reset)
		begin
			dout <= 8'h00;
		end
		else
		begin
			if(cs & ~we)
			begin
				if(addr)
					dout <= rx_dat;
				else
					dout <= status;
			end
		end
	end
	
	// diagnostics
	assign mbx_diag = {waddr,winc,raddr,rinc};

endmodule
