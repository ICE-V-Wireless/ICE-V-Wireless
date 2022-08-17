src = $(wildcard *.c)
obj = $(src:.c=.o)

CFLAGS = -Wall

send_c3usb: $(obj)
	$(CC) -o $@ $^

.PHONY: clean

clean:
	rm -f $(obj) send_c3usb