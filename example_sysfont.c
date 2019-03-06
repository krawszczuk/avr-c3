
#include <asf.h>
#include <util/delay.h>
#include <interrupt.h>

volatile char stan = 1;

//zamiana diody z guzika 1
ISR(PORTF_INT0_vect)
{
	if(1 == stan)
		stan = 0;
	else
		stan = 1;
}
//zmiana diod z guzika 2
ISR(PORTF_INT1_vect)
{
	stan = 2;
}

void spi_write(uint8_t gdzie, uint8_t co)
{
	PORTC.OUT = 0;
	_delay_us(100);
	
	SPIC.DATA = gdzie; //adres
	while(!(SPIC.STATUS==(1<<7)))
	{
		_delay_us(1);
	}
	
	SPIC.DATA = co; //wartosc
	while(!(SPIC.STATUS==(1<<7)))
	{
		_delay_us(1);
	}
	_delay_us(100);
	PORTC.OUT = (1<<4);
	return 0;
}

uint8_t spi_read()
{
	PORTC.OUT = 0;
	_delay_us(100);
	
	SPIC.DATA = (50 | (1<<7));//adres i read
	while(!(SPIC.STATUS==(1<<7)))
	{
		_delay_us(1);
	}
	
	SPIC.DATA = 0;
	while(!(SPIC.STATUS==(1<<7)))
	{
		_delay_us(1);
	}
	_delay_us(100);
	PORTC.OUT = (1<<4);
	return SPIC.DATA;
}

int main(void)
{
	cli();
	board_init();
	sysclk_init();
	
	//wybudzenie calej plytki
	PR.PRPA=0;
	PR.PRPB=0;
	PR.PRPC=0;
	PR.PRPD=0;
	PR.PRPE=0;
	PR.PRPF=0;
	
	//ustawienie adc
	ADCA.PRESCALER = 1;
	ADCA.CTRLB = 4;
	ADCA.REFCTRL = 1<<4;
	
	ADCA.CH0.CTRL=1;
	ADCA.CTRLA=1;
	    
		//wlaczenie i stan diod
		PORTD.DIR |= (1<<4)|(1<<5);
	    PORTD.OUT =0;
		PORTD.OUT =1<<5;

		//ustawienie pwm
	    TCD0.CTRLA|= (1<<0);
	    TCD0.CTRLB|= (1<<5)|(1<<4)|(1<<1)|(1<<0);
	    TCD0.PER = 220;
	    PORTD.REMAP = (1<<0)|(1<<1);
		
	//wlaczenie przerwan dla guzika 1 (port f 1)
	PMIC.CTRL = (1<<1);
	PORTF.INTCTRL |=  (1<<1); 
	PORTF.INT0MASK = (1<<1);   
	PORTF.PIN1CTRL = 3; 
	//dodanie drugiego guzika
	PORTF.INTCTRL |=  (1<<3);
	PORTF.INT1MASK = (1<<2); 
	PORTF.PIN2CTRL = 2; 
	
	//SPI setup
	PORTC.DIR = (1<<4)|(1<<5)|(1<<7); //wybor pinow
	SPIC.CTRL |= (1<<6)|(1<<4)|(1<<3)|(1<<2)|(1<<1);
	PORTC.OUT = (1<<4); //SS high
	_delay_ms(10);
	spi_write(45,(0|(1<<3))); //setup adxl

	sei();
	
	char a[3];
	char b[1];
	char c[3];
	int razem;
	uint8_t acel = 0;
	uint8_t wynik = 0;
	
	while (true)
	{
		ADCA.CTRLA |= 1<<2; //start pomiaru adc
		_delay_ms(100);
		gfx_mono_init();
		
		wynik = ADCA.CH0RES; //wynik adc

		itoa(wynik, a, 10);
		gfx_mono_draw_string(a, 0, 0, &sysfont);
		
		if(1 == stan)
		{
			TCD0.CCA = wynik; //swiecenie dioda zielona lub czerwona
			TCD0.CCB = 255;
		}
		else if(2 == stan)
		{
			TCD0.CCB = wynik;
			TCD0.CCA = wynik;
		}
		else
		{
			TCD0.CCA = 255;
			TCD0.CCB = wynik;
		}
		
		gfx_mono_draw_string("stan", 0, 10, &sysfont);//stan przerwan
		itoa(stan, b, 10);
		gfx_mono_draw_string(b, 30, 10, &sysfont);
		
		acel = spi_read();
		itoa(acel, c, 10);
		gfx_mono_draw_string(c, 0, 20, &sysfont);//przyspieszenie
		acel = spi_read(51);
		itoa(acel, c, 10);
		gfx_mono_draw_string(c, 20, 20, &sysfont);//przyspieszenie
	}
}
