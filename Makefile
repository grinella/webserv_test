FLAGS = -Wall -Wextra -Werror --std=c++98 -g -fsanitize=address
NAME = Webserv

all:
	@clear
	@echo Creating objects...
	@c++ $(FLAGS) src/*.cpp -o $(NAME)
	@echo Done ✓
	@echo "\033[0;31m    *    .  *     ✦    .         ★       .    *       ✦       .    * ✦           .     ★    "
	@echo "\033[0;33m     ____    __    ____  _______ .______        _______. _______ .______     ____    ____   "
	@echo "\033[0;93m .   \   \  /  \  /   / |   ____||   _  \      /       ||   ____||   _  \    \   \  /   /   "
	@echo "\033[0;32m      \   \/    \/   /  |  |__   |  |_)  |    |   (----\`|  |__   |  |_)  |    \   \/   /  ★ "
	@echo "\033[0;36m  *    \            /   |   __|  |   _  <      \   \    |   __|  |      /      \      / ✦   "
	@echo "\033[0;34m   ✦    \    /\    /    |  |____ |  |_)  | .----)   |   |  |____ |  |\  \----.  \    /      "
	@echo "\033[0;35m         \__/  \__/     |_______||______/  |_______/    |_______|| _| \`._____|   \__/       "
	@echo "\033[1;35m    ★        *    .    ✦      .      *      ✦         .     ★   * ✦  .  * ✦     .     ★     "
	@echo "\033[0m                             Created with <3 by grinella & ecaruso                             "
	@echo "\033[0m"

fclean:
	@clear
	@echo Cleaning...
	@-rm -f $(NAME)
	@-rm -rf $(NAME).dSYM
	@echo Done ✓

re: fclean all