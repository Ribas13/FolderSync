NAME = FolderSync
C_PP = c++ 
OBJS_D = objs
C_PPFLAGS = -Wall -Wextra -Werror -g #-fsanitize=address

SRC =	src/main.cpp		\
		src/FolderSync.cpp

OBJS = $(addprefix $(OBJS_D)/,$(SRC:%.cpp=%.o))


$(OBJS_D)/%.o:%.cpp
	@mkdir -p $(dir $@)
	@$(C_PP) $(C_PPFLAGS) -o $@ -c $<

all: $(NAME)

$(NAME): $(OBJS)
	@$(C_PP) $(C_PPFLAGS) $(OBJS) -o $(NAME)
	@echo -e "$(GREEN)Successfully built --> $(YELLOW)$(NAME)$(DEFAULT)"

clean:
	@echo -e "$(BYELLOW)Objects $(BRED)REMOVED$(DEFAULT)"
	@rm -rf $(OBJS)

fclean: clean
	@rm -rf $(NAME)
	@echo -e "$(BYELLOW)$(NAME) $(BRED)REMOVED$(DEFAULT)"

re: fclean all

##### COLORS #####

WHITE	= \033[0;37m
BLACK	= \033[0;30m
RED		= \033[0;31m
YELLOW	= \033[0;33m
BLUE	= \033[0;34m
GREEN	= \033[0;32m
DEFAULT = \033[0m

##### BOLD COLORS #####

BRED	= \033[31;1m
BGREEN	= \033[32;1m
BYELLOW	= \033[33;1m
BBLUE	= \033[34;1m
BMAGENTA	= \033[35;1m
BCYAN	= \033[36;1m
BWHITE	= \033[37;1m