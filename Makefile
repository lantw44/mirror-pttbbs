SUBDIR=	mbbsd util innbbsd
BBSHOME?=$(HOME)
OS!=uname
OSTYPE?=$(OS)

all install clean:
	@for i in $(SUBDIR); do\
		cd $$i;\
		$(MAKE) BBSHOME=$(BBSHOME) $@;\
		cd ..;\
	done
