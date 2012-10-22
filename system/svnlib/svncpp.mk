#
#
#
#
#

PROJECT = svncpp

BASEDIR = ../..

PATH +=:../../$(mode)/bin

include ../../make.common

INCLUDEPATH += \
\
	-I./../win32 \
	-I./../../system/include \
	-I./../../system/jlib \
	-I./../../system/scm \
	-I./../../system/mysql \
	-I./../../system/mysql/include \
	\
	-I/usr/include/subversion-1 \
	-I/usr/include/apr-1


#WIN32,_DEBUG,_MBCS,_LIB,APR_DECLARE_STATIC,APU_DECLARE_STATIC,SVN_DEBUG

debug_def += -DLOGMSGCOMPONENT=1 -D_USRDLL -DJLIB_EXPORTS \
		 -DMODULE_PRIORITY=13

release_def += -D_USRDLL -DLOGMSGCOMPONENT=1 -DJLIB_EXPORTS \
		 -DMODULE_PRIORITY=13

LDFLAGS +=  -shared -L../../$(mode)/libs

#LDFLAGS +=  -lcrypt

SRCS =  \
apr.cpp \
client.cpp \
client_annotate.cpp \
client_cat.cpp \
client_diff.cpp \
client_ls.cpp \
client_modify.cpp \
client_property.cpp \
client_status.cpp \
context.cpp \
datetime.cpp \
dirent.cpp \
entry.cpp \
exception.cpp \
info.cpp \
log_entry.cpp \
path.cpp \
pool.cpp \
property.cpp \
revision.cpp \
status.cpp \
status_selection.cpp \
targets.cpp \
url.cpp \
wc.cpp \
md5.cpp\
md5wrapper.cpp\
MySVN.cpp\
SVNContextListener.cpp\
SVNObjects.cpp

TARGET = ../../$(mode)/libs/libsvncpp.so

$(TARGET): $(DERIVED_SRCS) $(OBJS)
	@mkdir -p ../../$(mode)/libs
	@echo -e \\nLinking $@
	@$(CC) $(CCFLAGS) $(CCEXTRA) $(LDEXTRA) $(OBJS) $(LDFLAGS) -o $@ 

depend:
	@echo Making dependencies for svncpp
	@gcc -MM -MG $(CCDEFS) $(MDDEFS) $(COMMONINCLUDE) $(INCLUDEPATH) $(SRCS) | sed -e "s/^\(.*\.o\)/\$$\(OBJDIR\)\/\1/;" > svncpp.dep 

# -include svncpp.dep

clean: tidy
	$(RM) $(TARGET)

tidy:
	$(RM) $(OBJS)
	-rmdir -p $(OBJDIR)

#
# end of file.
#


