// wb_bus.v - wrapper for wishbone master and IP cores
// 07-01-19 E. Brombaugh

`default_nettype none

module wb_bus(
	input clk,				// system clock
	input rst,				// system reset
	input cs,				// chip select
	input we,				// write enable
	input [7:0] addr,		// register select
	input [7:0] din,		// data bus input
	output [7:0] dout,		// data bus output
	output rdy,				// high-true ready flag
	inout spi0_mosi,		// spi core 0 mosi
	inout spi0_miso,		// spi core 0 miso
	inout spi0_sclk,		// spi core 0 sclk
	inout spi0_cs0,			// spi core 0 cs
	inout spi1_mosi,		// spi core 1 mosi
	inout spi1_miso,		// spi core 1 miso
	inout spi1_sclk,		// spi core 1 sclk
	inout spi1_cs0,			// spi core 1 cs
	inout i2c0_sda,			// i2c core 0 data
	inout i2c0_scl			// i2c core 0 clock
);

	// the wishbone master
	wire sbstbi, sbrwi, sbacko;
	wire [7:0] sbadri, sbdato, sbdati;
	wb_master uwbm(
		.clk(clk),
		.rst(rst),
		.cs(cs),
		.we(we),
		.addr(addr),
		.din(din),
		.dout(dout),
		.rdy(rdy),
		.wb_stbo(sbstbi),
		.wb_adro(sbadri),
		.wb_rwo(sbrwi),
		.wb_dato(sbdati),
		.wb_acki(sbacko),
		.wb_dati(sbdato)
	);
	
	// SPI IP Core 0
	wire moe_0, mo_0, si_0;			// MOSI components
	wire soe_0, mi_0, so_0;			// MISO components
	wire sckoe_0, scko_0, scki_0;	// SCLK components
	wire mcsnoe_00, mcsno_00, scsni_0;		// CS0 components
	wire [7:0] sbdato_0;
	wire sbacko_0;
	SB_SPI #(
		.BUS_ADDR74("0b0000")
	) 
	spiInst0 (
		.SBCLKI(clk),
		.SBRWI(sbrwi),
		.SBSTBI(sbstbi),
		.SBADRI7(sbadri[7]),
		.SBADRI6(sbadri[6]),
		.SBADRI5(sbadri[5]),
		.SBADRI4(sbadri[4]),
		.SBADRI3(sbadri[3]),
		.SBADRI2(sbadri[2]),
		.SBADRI1(sbadri[1]),
		.SBADRI0(sbadri[0]),
		.SBDATI7(sbdati[7]),
		.SBDATI6(sbdati[6]),
		.SBDATI5(sbdati[5]),
		.SBDATI4(sbdati[4]),
		.SBDATI3(sbdati[3]),
		.SBDATI2(sbdati[2]),
		.SBDATI1(sbdati[1]),
		.SBDATI0(sbdati[0]),
		.MI(mi_0),
		.SI(si_0),
		.SCKI(scki_0),
		.SCSNI(scsni_0),		// must be pulled high to prevent SOE
		.SBDATO7(sbdato_0[7]),
		.SBDATO6(sbdato_0[6]),
		.SBDATO5(sbdato_0[5]),
		.SBDATO4(sbdato_0[4]),
		.SBDATO3(sbdato_0[3]),
		.SBDATO2(sbdato_0[2]),
		.SBDATO1(sbdato_0[1]),
		.SBDATO0(sbdato_0[0]),
		.SBACKO(sbacko_0),
		.SPIIRQ(),
		.SPIWKUP(),
		.SO(so_0),
		.SOE(soe_0),
		.MO(mo_0),
		.MOE(moe_0),
		.SCKO(scko_0),
		.SCKOE(sckoe_0),
		.MCSNO3(),
		.MCSNO2(),
		.MCSNO1(),
		.MCSNO0(mcsno_00),
		.MCSNOE3(),
		.MCSNOE2(),
		.MCSNOE1(),
		.MCSNOE0(mcsnoe_00)
	);
	
	// I/O drivers are tri-state output w/ simple input
	// MOSI driver
	SB_IO #(
		.PIN_TYPE(6'b101001),
		.PULLUP(1'b1),
		.NEG_TRIGGER(1'b0),
		.IO_STANDARD("SB_LVCMOS")
	) umosi_0 (
		.PACKAGE_PIN(spi0_mosi),
		.LATCH_INPUT_VALUE(1'b0),
		.CLOCK_ENABLE(1'b0),
		.INPUT_CLK(1'b0),
		.OUTPUT_CLK(1'b0),
		.OUTPUT_ENABLE(moe_0),
		.D_OUT_0(mo_0),
		.D_OUT_1(1'b0),
		.D_IN_0(si_0),
		.D_IN_1()
	);
	
	// MISO driver
	SB_IO #(
		.PIN_TYPE(6'b101001),
		.PULLUP(1'b1),
		.NEG_TRIGGER(1'b0),
		.IO_STANDARD("SB_LVCMOS")
	) umiso_0 (
		.PACKAGE_PIN(spi0_miso),
		.LATCH_INPUT_VALUE(1'b0),
		.CLOCK_ENABLE(1'b0),
		.INPUT_CLK(1'b0),
		.OUTPUT_CLK(1'b0),
		.OUTPUT_ENABLE(soe_0),
		.D_OUT_0(so_0),
		.D_OUT_1(1'b0),
		.D_IN_0(mi_0),
		.D_IN_1()
	);

	// SCK driver
	SB_IO #(
		.PIN_TYPE(6'b101001),
		.PULLUP(1'b1),
		.NEG_TRIGGER(1'b0),
		.IO_STANDARD("SB_LVCMOS")
	) usclk_0 (
		.PACKAGE_PIN(spi0_sclk),
		.LATCH_INPUT_VALUE(1'b0),
		.CLOCK_ENABLE(1'b0),
		.INPUT_CLK(1'b0),
		.OUTPUT_CLK(1'b0),
		.OUTPUT_ENABLE(sckoe_0),
		.D_OUT_0(scko_0),
		.D_OUT_1(1'b0),
		.D_IN_0(scki_0),
		.D_IN_1()
	);

	// CS0 driver
	SB_IO #(
		.PIN_TYPE(6'b101001),
		.PULLUP(1'b1),
		.NEG_TRIGGER(1'b0),
		.IO_STANDARD("SB_LVCMOS")
	) ucs0_0 (
		.PACKAGE_PIN(spi0_cs0),
		.LATCH_INPUT_VALUE(1'b0),
		.CLOCK_ENABLE(1'b0),
		.INPUT_CLK(1'b0),
		.OUTPUT_CLK(1'b0),
		.OUTPUT_ENABLE(1'b1),	// or mcsnoe_00 for hi-z when inactive
		.D_OUT_0(mcsno_00),
		.D_OUT_1(1'b0),
		.D_IN_0(scsni_0),		// unused to prevent accidental slave mode
		.D_IN_1()
	);
	
	// SPI IP Core 1
	wire moe_1, mo_1, si_1;			// MOSI components
	wire soe_1, mi_1, so_1;			// MISO components
	wire sckoe_1, scko_1, scki_1;	// SCLK components
	wire mcsnoe_01, mcsno_01, scsni_1;		// CS0 components
	wire [7:0] sbdato_1;
	wire sbacko_1;
	SB_SPI #(
		.BUS_ADDR74("0b0010")	// 2nd SPI instance (despite Lattice docs)
	) 
	spiInst1 (
		.SBCLKI(clk),
		.SBRWI(sbrwi),
		.SBSTBI(sbstbi),
		.SBADRI7(sbadri[7]),
		.SBADRI6(sbadri[6]),
		.SBADRI5(sbadri[5]),
		.SBADRI4(sbadri[4]),
		.SBADRI3(sbadri[3]),
		.SBADRI2(sbadri[2]),
		.SBADRI1(sbadri[1]),
		.SBADRI0(sbadri[0]),
		.SBDATI7(sbdati[7]),
		.SBDATI6(sbdati[6]),
		.SBDATI5(sbdati[5]),
		.SBDATI4(sbdati[4]),
		.SBDATI3(sbdati[3]),
		.SBDATI2(sbdati[2]),
		.SBDATI1(sbdati[1]),
		.SBDATI0(sbdati[0]),
		.MI(mi_1),
		.SI(si_1),
		.SCKI(scki_1),
		.SCSNI(scsni_1),		// must be pulled high to prevent SOE
		.SBDATO7(sbdato_1[7]),
		.SBDATO6(sbdato_1[6]),
		.SBDATO5(sbdato_1[5]),
		.SBDATO4(sbdato_1[4]),
		.SBDATO3(sbdato_1[3]),
		.SBDATO2(sbdato_1[2]),
		.SBDATO1(sbdato_1[1]),
		.SBDATO0(sbdato_1[0]),
		.SBACKO(sbacko_1),
		.SPIIRQ(),
		.SPIWKUP(),
		.SO(so_1),
		.SOE(soe_1),
		.MO(mo_1),
		.MOE(moe_1),
		.SCKO(scko_1),
		.SCKOE(sckoe_1),
		.MCSNO3(),
		.MCSNO2(),
		.MCSNO1(),
		.MCSNO0(mcsno_01),
		.MCSNOE3(),
		.MCSNOE2(),
		.MCSNOE1(),
		.MCSNOE0(mcsnoe_01)
	);
	
	// I/O drivers are tri-state output w/ simple input
	// MOSI driver
	SB_IO #(
		.PIN_TYPE(6'b101001),
		.PULLUP(1'b1),
		.NEG_TRIGGER(1'b0),
		.IO_STANDARD("SB_LVCMOS")
	) umosi_1 (
		.PACKAGE_PIN(spi1_mosi),
		.LATCH_INPUT_VALUE(1'b0),
		.CLOCK_ENABLE(1'b0),
		.INPUT_CLK(1'b0),
		.OUTPUT_CLK(1'b0),
		.OUTPUT_ENABLE(moe_1),
		.D_OUT_0(mo_1),
		.D_OUT_1(1'b0),
		.D_IN_0(si_1),
		.D_IN_1()
	);
	
	// MISO driver
	SB_IO #(
		.PIN_TYPE(6'b101001),
		.PULLUP(1'b1),
		.NEG_TRIGGER(1'b0),
		.IO_STANDARD("SB_LVCMOS")
	) umiso_1 (
		.PACKAGE_PIN(spi1_miso),
		.LATCH_INPUT_VALUE(1'b0),
		.CLOCK_ENABLE(1'b0),
		.INPUT_CLK(1'b0),
		.OUTPUT_CLK(1'b0),
		.OUTPUT_ENABLE(soe_1),
		.D_OUT_0(so_1),
		.D_OUT_1(1'b0),
		.D_IN_0(mi_1),
		.D_IN_1()
	);

	// SCK driver
	SB_IO #(
		.PIN_TYPE(6'b101001),
		.PULLUP(1'b1),
		.NEG_TRIGGER(1'b0),
		.IO_STANDARD("SB_LVCMOS")
	) usclk_1 (
		.PACKAGE_PIN(spi1_sclk),
		.LATCH_INPUT_VALUE(1'b0),
		.CLOCK_ENABLE(1'b0),
		.INPUT_CLK(1'b0),
		.OUTPUT_CLK(1'b0),
		.OUTPUT_ENABLE(sckoe_1),
		.D_OUT_0(scko_1),
		.D_OUT_1(1'b0),
		.D_IN_0(scki_1),
		.D_IN_1()
	);

	// CS0 driver
	SB_IO #(
		.PIN_TYPE(6'b101001),
		.PULLUP(1'b1),
		.NEG_TRIGGER(1'b0),
		.IO_STANDARD("SB_LVCMOS")
	) ucs0_1 (
		.PACKAGE_PIN(spi1_cs0),
		.LATCH_INPUT_VALUE(1'b0),
		.CLOCK_ENABLE(1'b0),
		.INPUT_CLK(1'b0),
		.OUTPUT_CLK(1'b0),
		.OUTPUT_ENABLE(1'b1),	// or mcsnoe_00 for hi-z when inactive
		.D_OUT_0(mcsno_01),
		.D_OUT_1(1'b0),
		.D_IN_0(scsni_1),		// unused to prevent accidental slave mode
		.D_IN_1()
	);
	
	// I2C IP Core
	wire sda_oe_0, sda_i_0, sda_o_0;			// sda components
	wire scl_oe_0, scl_i_0, scl_o_0;			// scl components
	wire [7:0] sbdato_2;
	wire sbacko_2;
	SB_I2C #(
		.BUS_ADDR74("0b0001")
	) 
	i2cInst0 (
		.SBCLKI(clk),
		.SBRWI(sbrwi),
		.SBSTBI(sbstbi),
		.SBADRI7(sbadri[7]),
		.SBADRI6(sbadri[6]),
		.SBADRI5(sbadri[5]),
		.SBADRI4(sbadri[4]),
		.SBADRI3(sbadri[3]),
		.SBADRI2(sbadri[2]),
		.SBADRI1(sbadri[1]),
		.SBADRI0(sbadri[0]),
		.SBDATI7(sbdati[7]),
		.SBDATI6(sbdati[6]),
		.SBDATI5(sbdati[5]),
		.SBDATI4(sbdati[4]),
		.SBDATI3(sbdati[3]),
		.SBDATI2(sbdati[2]),
		.SBDATI1(sbdati[1]),
		.SBDATI0(sbdati[0]),
		.SCLI(scl_i_0),
		.SDAI(sda_i_0),
		.SBDATO7(sbdato_2[7]),
		.SBDATO6(sbdato_2[6]),
		.SBDATO5(sbdato_2[5]),
		.SBDATO4(sbdato_2[4]),
		.SBDATO3(sbdato_2[3]),
		.SBDATO2(sbdato_2[2]),
		.SBDATO1(sbdato_2[1]),
		.SBDATO0(sbdato_2[0]),
		.SBACKO(sbacko_2),
		.I2CIRQ(),
		.I2CWKUP(),
		.SCLO(scl_o_0),
		.SCLOE(scl_oe_0),
		.SDAO(sda_o_0),
		.SDAOE(sda_oe_0)
	);
	
	// I/O drivers are tri-state output w/ simple input
	// SDA driver
	SB_IO #(
		.PIN_TYPE(6'b101001),
		.PULLUP(1'b1),
		.NEG_TRIGGER(1'b0),
		.IO_STANDARD("SB_LVCMOS")
	) usda (
		.PACKAGE_PIN(i2c0_sda),
		.LATCH_INPUT_VALUE(1'b0),
		.CLOCK_ENABLE(1'b0),
		.INPUT_CLK(1'b0),
		.OUTPUT_CLK(1'b0),
		.OUTPUT_ENABLE(sda_oe_0),
		.D_OUT_0(sda_o_0),
		.D_OUT_1(1'b0),
		.D_IN_0(sda_i_0),
		.D_IN_1()
	);
	
	// SCL driver
	SB_IO #(
		.PIN_TYPE(6'b101001),
		.PULLUP(1'b1),
		.NEG_TRIGGER(1'b0),
		.IO_STANDARD("SB_LVCMOS")
	) uscl (
		.PACKAGE_PIN(i2c0_scl),
		.LATCH_INPUT_VALUE(1'b0),
		.CLOCK_ENABLE(1'b0),
		.INPUT_CLK(1'b0),
		.OUTPUT_CLK(1'b0),
		.OUTPUT_ENABLE(scl_oe_0),
		.D_OUT_0(scl_o_0),
		.D_OUT_1(1'b0),
		.D_IN_0(scl_i_0),
		.D_IN_1()
	);
	
	// OR muxing of output data & ack
	assign sbdato = sbdato_0 | sbdato_1 | sbdato_2;
	assign sbacko = sbacko_0 | sbacko_1 | sbacko_2;
endmodule
