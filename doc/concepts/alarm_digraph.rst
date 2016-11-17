ALARM
*****

.. graphviz::

    digraph{
        interlock_ack
        [
            shape = triangle
            style = filled
            fillcolor = purple
            label = "I"
            fontsize = 24
            fixedsize = true
            penwidth = 5
            fontname = "sans-serif"
        ]

        interlock
        [
            shape = triangle
            color = purple
            style = filled
            fillcolor = purple
            label = "I"
            fontsize = 24
            fixedsize = true
            fontname = "sans-serif"

        ]
    }

.. graphviz::

    digraph{
        critical_ack
        [
            shape = hexagon
            style = filled
            fillcolor = red
            label = "C"
            fontsize = 24
            fixedsize = true
            penwidth = 5
            fontname = "sans-serif"
        ]

        critical
        [
            shape = hexagon
            color = red
            style = filled
            fillcolor = red
            label = "C"
            fontsize = 24
            fixedsize = true
            fontname = "sans-serif"
        ]
    }

.. graphviz::

    digraph{
        warning_ack
        [
            shape = diamond
            style = filled
            fillcolor = yellow
            label = "W"
            fontsize = 24
            fixedsize = true
            penwidth = 5
            fontname = "sans-serif"
        ]

        warning
        [
            shape = diamond
            color = yellow
            label = "W"
            fontsize = 24
            fixedsize = true
            style = filled
            fillcolor = yellow
            fontname = "sans-serif"
        ]
    }