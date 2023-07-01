library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

--
--This SPI master is taken from:
--https://gitlab.fel.cvut.cz/canbus/zynq/zynq-can-sja1000-top/blob/microzed_apo/system/ip/spi_leds_and_enc_1.0/hdl/spi_leds_and_enc_v1_0_spi_fsm.vhd
--

entity spi_master is
	generic (
		data_width	: integer	:= 64;
		spi_clkdiv	: integer	:= 1
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
end spi_master;

architecture arch_imp of spi_master is
	signal	shift_reg	: std_logic_vector(data_width-1 downto 0):=(others=>'0');
	signal	data_cnt	: natural range 0 to data_width :=0;
	signal	div_cnt		: natural range 0 to spi_clkdiv-1:=spi_clkdiv-1;
	signal	clk_phase	: std_logic:='0';
	signal	in_progress	: std_logic:='0';

begin

	process is
	begin
	  wait until rising_edge (clk_in);
	  if ( reset_in = '1' ) then
	    shift_reg <= (others => '0');
	    rx_data <= (others => '0');
	    div_cnt   <= spi_clkdiv-1;
	    transfer_ready <= '0';
	    data_cnt  <= 0;
	    spi_clk   <= '1';
	    spi_cs    <= '1';
	    spi_mosi  <= '0';
	    clk_phase <= '0';
	    in_progress <= '0';
	  elsif (clk_en = '1') then
	    transfer_ready <= '0';
	    if (div_cnt /= 0) then
	      div_cnt <= div_cnt - 1;
	    elsif clk_phase = '1' then
	      div_cnt   <= spi_clkdiv-1;
	      clk_phase <= '0';
	      spi_clk   <= '1';
	      if in_progress = '1' then
	        shift_reg(data_width-1) <= spi_miso;
	      end if;
	    else
	      clk_phase <= '1';
	      div_cnt   <= spi_clkdiv-1;
	      spi_clk   <= '0';
	      if (data_cnt = 0) then
	        spi_cs  <= '1';
	        if in_progress = '1' then
	          spi_cs    <= '1';
	          rx_data   <= shift_reg;
	        end if;
	        transfer_ready <= in_progress;
	        in_progress <= '0';
	        if (trasfer_rq = '1') then
	          shift_reg <= tx_data;
	          data_cnt  <= data_width;
	          spi_mosi  <= '0';
	        end if;
	      else
	        in_progress <= '1';
	        spi_cs    <= '0';
	        spi_mosi  <= shift_reg(0);
	        shift_reg <= '0' & shift_reg(data_width-1 downto 1);
	        data_cnt  <= data_cnt - 1;
	      end if;
	    end if;
	  end if;
	end process;

end arch_imp;
