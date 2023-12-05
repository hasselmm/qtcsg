macro(qtcsg_show_variable variable)
    message(STATUS "${variable}: ${${variable}}")
endmacro()

macro(qtcsg_show_property target property)
    get_target_property(${property} ${target} ${property})
    message(STATUS "${target} ${property}: ${${property}}")
endmacro()

