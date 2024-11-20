CC=$(CROSS_COMPILE)gcc
CFLAGS ?= -Wall
INCLUDES ?= 
OBJS ?= aesdsocket.c
TARGET ?= aesdsocket
LDFLAGS ?= -Wall -Wextra
#-Werror -

.phony:		default
default:	all

.phony:		all
all:		$(APP)
			$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) -o $(TARGET) $(LDFLAGS)
			@echo "Done"

.phony:		clean
clean:
			-@rm *.o $(TARGET) true 1>/dev/null 2>/dev/null || true
			@echo "Clean."
