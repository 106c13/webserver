NAME=webserv

SRC_MAIN = $(addprefix srcs/main/, main.cpp)

SRC = $(SRC_MAIN)

OBJ=$(SRC:.cpp=.o)

CPP=c++

CPPFLAGS=-Wall -Wextra -Werror -std=c++98 -I include

all: $(NAME)

%.o: %.cpp
	$(CPP) $(CPPFLAGS) -c $< -o $@

$(NAME): $(OBJ)
	$(CPP) $(CPPFLAGS) $^ -o $(NAME)
	@echo "Build complete: ./$(NAME)"

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
