product iconbar
    id "iconbar 0.8 - a desktop utility for managing icons"
    image sw
        id "Software"
        version 4
        order 9999
        subsys mips4 default
            id "MIPS4 and Motif 2.1 Base Software"
            replaces self
            incompat iconbar.sw.mips3 0 maxint
            exp iconbar.sw.mips4
        endsubsys
        subsys mips3
            id "MIPS3 and Motif 1.2 Base Software"
            replaces self
            incompat iconbar.sw.mips4 0 maxint
            exp iconbar.sw.mips3
        endsubsys
    endimage
    image man
        id "Man Page"
        version 4
        order 9999
        subsys man1 default
            id "User Command Man Page"
            replaces self
            exp iconbar.man.man1
        endsubsys
    endimage
    image optional
        id "Optional Software"
        version 4
        order 9999
        subsys src
            id "Source Code"
            replaces self
            exp iconbar.optional.src
        endsubsys
        subsys masks
            id "Masks"
            replaces self
            exp iconbar.optional.masks
        endsubsys
        subsys filetypes
            id "File Type Rules"
            replaces self
            exp iconbar.optional.filetypes
        endsubsys
    endimage
endproduct
