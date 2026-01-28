NAME = webserv

# ---------- Temprorary -----------
CGI_SRC = cgi/test.c
CGI_BIN = php-cgi
CC = cc
CFLAGS = -Wall -Wextra -Werror
# ---------------------------------

SRCS_DIR = srcs/
OBJS_DIR = obj/
INCLUDES = include/

SRC_MAIN   = main.cpp
SRC_SERVER = server.cpp network.cpp utils.cpp \
             defaults.cpp path_resolver.cpp \
			 autoindex.cpp cgi.cpp header_generator.cpp
SRC_REQUEST = request.cpp network.cpp
SRC_PARSER = ConfigParser.cpp RequestParser.cpp

SRC = \
    $(addprefix $(SRCS_DIR)main/, $(SRC_MAIN)) \
    $(addprefix $(SRCS_DIR)server/, $(SRC_SERVER)) \
    $(addprefix $(SRCS_DIR)request/, $(SRC_REQUEST)) \
    $(addprefix $(SRCS_DIR)parser/, $(SRC_PARSER))

OBJ = $(patsubst $(SRCS_DIR)%.cpp, $(OBJS_DIR)%.o, $(SRC))

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -I $(INCLUDES)


GREEN = \033[1;32m
YELLOW = \033[1;33m
BLUE = \033[1;34m
RED = \033[1;31m
CYAN = \033[1;36m
RESET = \033[0m

all: $(NAME) $(CGI_BIN)

# ---------- Temprorary -----------
$(CGI_BIN): $(CGI_SRC)
	@echo "$(CYAN)[Compiling CGI]$(RESET) $<"
	@$(CC) $(CFLAGS) $< -o $(CGI_BIN)
	@echo "$(GREEN)✅ CGI ready: ./$(CGI_BIN)$(RESET)"
# ---------------------------------
#
$(NAME): $(OBJ)
	@echo "$(YELLOW)🔧 Linking $(NAME)...$(RESET)"
	@$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)
	@echo "$(GREEN)✅ Build complete: ./$(NAME)$(RESET)"

$(OBJS_DIR)%.o: $(SRCS_DIR)%.cpp
	@mkdir -p $(dir $@)
	@echo "$(CYAN)[Compiling]$(RESET) $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(NAME): $(OBJ)
	@echo "$(YELLOW)🔧 Linking WebServer...$(RESET)"
	@$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)
	@echo "$(GREEN)Build complete: ./$(NAME)$(RESET)"
	@echo "$(BLUE)------------------------------------------------------------------------------$(RESET)"
	@echo "$(BLUE)░██╗░░░░░░░██╗███████╗██████╗░░██████╗███████╗██████╗░██╗░░░██╗███████╗██████╗░$(RESET)"
	@echo "$(BLUE)░██║░░██╗░░██║██╔════╝██╔══██╗██╔════╝██╔════╝██╔══██╗██║░░░██║██╔════╝██╔══██╗$(RESET)"
	@echo "$(BLUE)░╚██╗████╗██╔╝█████╗░░██████╦╝╚█████╗░█████╗░░██████╔╝╚██╗░██╔╝█████╗░░██████╔╝$(RESET)"
	@echo "$(BLUE)░░████╔═████║░██╔══╝░░██╔══██╗░╚═══██╗██╔══╝░░██╔══██╗░╚████╔╝░██╔══╝░░██╔══██╗$(RESET)"
	@echo "$(BLUE)░░╚██╔╝░╚██╔╝░███████╗██████╦╝██████╔╝███████╗██║░░██║░░╚██╔╝░░███████╗██║░░██║$(RESET)"
	@echo "$(BLUE)░░░╚═╝░░░╚═╝░░╚══════╝╚═════╝░╚═════╝░╚══════╝╚═╝░░╚═╝░░░╚═╝░░░╚══════╝╚═╝░░╚═╝$(RESET)"
	@echo "$(RED)                       🔥 MADE BY: Arseniy & Hakob 🔥         $(RESET)"
	@echo "$(BLUE)-------------------------------------------------------------------------------$(RESET)"

test:
	@$(CXX) $(CXXFLAGS) tests/test_config_parser.cpp $(SRCS_DIR)parser/ConfigParser.cpp -o test_config_parser
	@$(CXX) $(CXXFLAGS) tests/test_request_parser.cpp $(SRCS_DIR)parser/RequestParser.cpp -o test_request_parser
	@./test_config_parser ; c=$$? ; echo "" ; ./test_request_parser ; r=$$? ; rm -f test_config_parser test_request_parser ; exit $$((c + r))

clean:
	@rm -rf $(OBJS_DIR)
	@echo "$(RED)🧹 Object files removed!$(RESET)"

fclean: clean
	@rm -rf $(NAME) $(CGI_BIN)
	@echo "$(RED)🔥 Executables removed: $(NAME), $(CGI_BIN)$(RESET)"

re: fclean all

.PHONY: all clean fclean re test
