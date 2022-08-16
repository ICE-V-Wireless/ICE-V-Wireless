src = $(wildcard *.c)
obj = $(src:.c=.o)

send_c3usb: $(obj)
	$(CC) -o $@ $^

.PHONY: clean

clean:
	rm -f $(obj) send_c3usb