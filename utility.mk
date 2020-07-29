
# Color table: https://en.wikipedia.org/wiki/ANSI_escape_code#8-bit
NORMFONT    :=  $(shell tput sgr0)
RED         :=  $(shell tput setaf 9)
GREEN       :=  $(shell tput setaf 2)
YELLOW      :=  $(shell tput setaf 11)
BLUE        :=  $(shell tput setaf 12)

# The run_and_test function runs a given command $(1) in a pretty, colored way,
# omitting boilerplate and only showing success or failure plus any warnings.
# Use $(2) to indicate what is being done (default: Compiling)
# see also http://www.lunderberg.com/2015/08/25/cpp-makefile-pretty-output/
define run_and_test
  printf "$(BLUE)$(if $(2),$(2),Compiling) $(@F)$(NORMFONT)...\r";          \
  $(1) 2> $@.log;                                                           \
  RESULT=$$?;                                                               \
  if [ $$RESULT -ne 0 ]; then                                               \
    printf "%-70b%b" 																												\
			"$(BLUE)$(if $(2),$(2),Compiling) $@" 																\
			"$(RED)[ERROR]$(NORMFONT)\n"; 																				\
  elif [ -s $@.log ]; then                                                  \
    printf "%-70b%b" 																												\
			"$(BLUE)$(if $(2),$(2),Compiling) $@" 																\
			"$(YELLOW)[WARNING]$(NORMFONT)\n"; 																		\
  else                                                                      \
    printf "%-70b%b" 																												\
		"$(BLUE)$(if $(2),$(2),Compiling) $(@F)" 																\
		"$(GREEN)[OK]$(NORMFONT)\n"; 																						\
  fi;                                                                       \
  cat $@.log;                                                               \
  rm -f $@.log;                                                             \
  exit $$RESULT
endef

# The sync_git function syncs the whole directory (minus files in .gitignore)
# to the location given by $(1) and shows its output in a neat way.
define sync_git
  printf "$(BLUE)rsync the git repo to \"$(1)\"...$(NORMFONT)\n";         	\
  rsync -auhzvi --delete	 																									\
		--exclude-from=.gitignore --exclude=".git" --exclude="rapidjson" -e ssh	\
		./ $(1)                                                             		\
		| sed -n -e 's/^[<>ch].* //p' -e 's/^\*//p'; 														\
  RESULT=$$?;                                                             	\
  if [ $$RESULT -ne 0 ]; then                                             	\
    printf "%-60b%b" "$(BLUE)rsync " "$(RED)[ERROR]$(NORMFONT)\n";      		\
  else                                                                    	\
    printf "%-60b%b" "$(BLUE)rsync" "$(GREEN)[OK]$(NORMFONT)\n";        		\
  fi;                                                                     	\
  exit $$RESULT
	# Details: The rsync command puts out a string before every line that specifies
	# the action on the corresponding file, starting with either <, >, c, or h
	# if there are actual changes to files. (see rsync --itemize-changes option).
	# The sed command filters out those lines, so that lines starting with
	# "." (rsync did nothing) and the "header" and "footer" are not displayed.
	# Lines starting with a literal * are treated specially as they consist
	# of messages (like deleting a file)
endef
