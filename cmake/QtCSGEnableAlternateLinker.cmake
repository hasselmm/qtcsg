include(CheckLinkerFlag)

# Checks if alternate, faster linkers like mold, lld, or gold
# are supported by the compiler and enables the best one found.
function(qtcsg_enable_alternate_linker)
    foreach(linker_name IN ITEMS mold lld gold)
        string(TOUPPER QTCSG_HAVE_ALTERNATE_LINKER_${linker_name} linker_variable)
        check_linker_flag(CXX -fuse-ld=${linker_name} ${linker_variable})

        if (${linker_variable})
            message(STATUS "Using alternate linker: ${linker_name}")
            add_link_options(-fuse-ld=${linker_name})
            break()
        endif()
    endforeach()
endfunction()
