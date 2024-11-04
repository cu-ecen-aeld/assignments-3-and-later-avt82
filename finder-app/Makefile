APP=writer
CC=$(CROSS_COMPILE)gcc
CC_FLAGS=-Wall
CC_INCLUDE=

.phony:		all
all:		$(APP)
		@echo "Build completed ($(CC))"

.phony:		clean
clean:
		-@rm *.o *.so $(APP) 1>/dev/null 2>/dev/null
		@echo "Done."

$(APP):		$(APP).c
		@$(CC) $(CC_FLAGS) $(^) -o $(@)
		@echo "Compiled $(^)"
