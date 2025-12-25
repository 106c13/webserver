NAME = webserv

SRCS_DIR = srcs/
OBJS_DIR = obj/
INCLUDES = include

SRC_MAIN   = main.cpp
SRC_SERVER = server.cpp network.cpp utils.cpp \
			 defaults.cpp
SRC_REQUEST = request.cpp network.cpp

SRC = \
	$(addprefix $(SRCS_DIR)main/, $(SRC_MAIN)) \
	$(addprefix $(SRCS_DIR)server/, $(SRC_SERVER)) \
	$(addprefix $(SRCS_DIR)request/, $(SRC_REQUEST))

OBJ = $(patsubst $(SRCS_DIR)%.cpp, $(OBJS_DIR)%.o, $(SRC))

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -I $(INCLUDES)

GREEN = \033[1;32m
YELLOW = \033[1;33m
BLUE = \033[1;34m
RED = \033[1;31m
CYAN = \033[1;36m
RESET = \033[0m

all: $(NAME)

$(NAME): $(OBJ)
	@echo "$(YELLOW)🔧 Linking $(NAME)...$(RESET)"
	@$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)
	@echo "$(GREEN)✅ Build complete: ./$(NAME)$(RESET)"

# Compile rule: srcs/.../*.cpp → obj/.../*.o
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
	@echo "$(RED)                       🔥 MADE BY: Arseniy & Hakob 🔥		 $(RESET)"
	@echo "$(BLUE)-------------------------------------------------------------------------------$(RESET)"

clean:
	@rm -rf $(OBJS_DIR)
	@echo "$(RED)🧹 Object files removed!$(RESET)"

fclean: clean
	@rm -rf $(NAME)
	@echo "$(RED)🔥 Executable removed: $(NAME)$(RESET)"

re: fclean all

.PHONY: all clean fclean re
