library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

--`default_nettype none
--`define DIV 14'd9999

entity psram_demo is
	port(
		spi0_mosi:out std_logic;--these will be automatically bound to PSRAM SPI because of .pcf file 		
		spi0_miso:in std_logic;
		spi0_sclk:out std_logic;
		spi0_cs0:out std_logic
		);
end;
architecture rtl of psram_demo is
	component SB_HFOSC is port(
		CLKHFEN	:   in  std_logic;
		CLKHFPU	:   in  std_logic;
		CLKHF	:   out  std_logic
		);
	end component;

	component spi_master is
	generic (
		data_width	: integer	:= 64;
		spi_clkdiv	: integer	:= 2
	);
	port (
		reset_in   	: in std_logic;

		clk_in   	: in std_logic;
		clk_en   	: in std_logic;

		spi_clk   	: out std_logic;
		spi_cs   	: out std_logic;
		spi_mosi   	: out std_logic;
		spi_miso  	: in std_logic;

		tx_data		: in std_logic_vector(data_width-1 downto 0);
		rx_data		: out std_logic_vector(data_width-1 downto 0);

		trasfer_rq	: in std_logic;
		transfer_ready	: out std_logic
	);
	end component;

function reverse_vector (a: in std_logic_vector)return std_logic_vector is
	variable result: std_logic_vector(a'RANGE);
	alias aa: std_logic_vector(a'REVERSE_RANGE) is a;
begin
	for i in aa'RANGE loop
		result(i) := aa(i);
	end loop;
	return result;
end;	
--0x02 is write command, 0x000000 is address, 0x57575757 is data to write
--Documentation here: http://www.lyontek.com/pdf/ddr/LY68L6400-1.2.pdf
signal PSRAM_SEND:std_logic_vector(63 downto 0):=x"0200000057575757";
signal CMP:std_logic_vector(15 downto 0):=x"270F";
signal CLKHF:std_logic;
signal clkdiv:std_logic_vector(15 downto 0):=x"0000";
--signal rx_data_trash:std_logic_vector(63 downto 0);
signal spi_reset:std_logic:='1';
begin	
	OSCInst2: SB_HFOSC port map(--high frequency clock, documentation here: https://www.latticesemi.com/~/media/LatticeSemi/Documents/ApplicationNotes/IK/iCE40OscillatorUsageGuide.pdf
		CLKHFEN=>'1',
		CLKHFPU=>'1',
		CLKHF=>CLKHF
		);	
	
	psram: spi_master port map(--SPI master that controls the PSRAM
		reset_in=>spi_reset,
		clk_in=>CLKHF,
		clk_en=>'1',
		spi_clk =>  spi0_sclk,
		spi_cs   =>	spi0_cs0,
		spi_mosi   =>	spi0_mosi,
		spi_miso  	=>spi0_miso,
		tx_data=>reverse_vector(PSRAM_SEND),
		rx_data=>open,--rx_data_trash,--here one could get read data from psram						
		trasfer_rq=>'1',--transfer request fixed 1 to repeat the same transmission
		transfer_ready=>open--this can be used to detect transfer done
		);

	process (CLKHF,clkdiv)begin--this is to reset the SPI master at startup		
		if rising_edge(CLKHF) then
			if unsigned(clkdiv) >= unsigned(CMP) then
				if spi_reset='1' then spi_reset<='0'; end if;
			else
				clkdiv <= std_logic_vector(unsigned(clkdiv) + 1);
			end if;
		end if;
	end process;	
end;
