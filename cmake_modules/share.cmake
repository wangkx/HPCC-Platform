###########################################################################

# Source_groups: organize files in Folder in Visual Studio

# VS IDE Folder Names
set(GENERATED_CPP "Generated CPP Files") 
set(GENERATED_OTHER "Generated Files")
set(PRECOMPILE_FILES "Precompile Files")
set(NCM_FILES "NCM Source Files")
set(ECM_FILES "ECM Source Files")
set(SCM_FILES "SCM Source Files")
set(CMAKE_MODULES "CMake Modules")
set(HID_FILES "HID Files")

# groups
source_group("Impl. Header Files" REGULAR_EXPRESSION .*[.]ipp$|.*[.]tpp$|.*[.]inc$)

source_group(${ECM_FILES} REGULAR_EXPRESSION  .*[.]ecm$)
source_group(${NCM_FILES} REGULAR_EXPRESSION  .*[.]ncm$)
source_group(${SCM_FILES} REGULAR_EXPRESSION  .*[.]scm$)
source_group(${HID_FILES} REGULAR_EXPRESSION  .*[.]hid$)
source_group(${CMAKE_MODULES} REGULAR_EXPRESSION  .*[.]cmake$)
#source_group(${PRECOMPILE_FILES} REGULAR_EXPRESSION  .*_pch[.]h$)
#source_group(${PRECOMPILE_FILES} REGULAR_EXPRESSION  .*_pch[.]cpp$)


###########################################################################
