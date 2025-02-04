FLAGS = -Wall -Wextra -Werror --std=c++98
NAME = webserv
SRC = $(wildcard src/*.cpp)
OBJ = $(SRC:.cpp=.o)

all: $(NAME)
	@echo "\033[0;31m    *    .  *     ✦    .         ★       .    *       ✦       .    * ✦           .     ★    "
	@echo "\033[0;33m     ____    **    **__  _______ .______        _______. _______ .______     ____    ____   "
	@echo "\033[0;93m .   \   \  /  \  /   / |   ____||   *  \      /       ||   *___||   _  \    \   \  /   /   "
	@echo "\033[0;32m      \   \/    \/   /  |  |__   |  |_)  |    |   (----\`|  |__   |  |_)  |    \   \/   /  ★ "
	@echo "\033[0;36m  *    \            /   |   __|  |   *  <      \   \    |   *_|  |      /      \      / ✦   "
	@echo "\033[0;34m   ✦    \    /\    /    |  |____ |  |_)  | .----)   |   |  |____ |  |\  \----.  \    /      "
	@echo "\033[0;35m         \__/  \__/     |_______||______/  |_______/    |_______|| *| \`.*____|   \__/       "
	@echo "\033[1;35m    ★        *    .    ✦      .      *      ✦         .     ★   * ✦  .  * ✦     .     ★     "
	@echo "\033[0m                             Created with <3 by grinella & ecaruso                             "
	@echo "\033[0m"

$(NAME): $(OBJ)
	@echo "Creating executable..."
	@c++ $(FLAGS) $(OBJ) -o $(NAME)
	@echo "Done ✓"

%.o: %.cpp
	@echo "Compiling $<..."
	@c++ $(FLAGS) -c $< -o $@

clean:
	@echo "Cleaning object files..."
	@rm -f $(OBJ)
	@echo "Done ✓"

fclean: clean
	@echo "Cleaning executable..."
	@rm -f $(NAME)
	@rm -rf $(NAME).dSYM
	@echo "Done ✓"

re: fclean all

.PHONY: all clean fclean re