// ice-v spi passthru design for PSRAM access
// 06-22-22 E. Brombaugh

`default_nettype none

module spi_pass(
	// 12MHz external osc
	input clk_12MHz,
		
	// RISCV SPI
	output	spi0_mosi,		// SPI core 0
	input	spi0_miso,
	output	spi0_sclk,
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
	//------------------------------
	// SPI pass-thru
	//------------------------------
	assign spi0_mosi = SPI_MOSI;
	assign SPI_MISO = spi0_miso;
	assign spi0_sclk = SPI_SCLK;
	assign spi0_cs0 = SPI_CSL;
	assign spi0_nwp = 1'b1;
	assign spi0_nhld = 1'b1;
	
	//------------------------------
	// everything else is default
	//------------------------------
endmodule
