SECTIONS
{
    .rel.rodata.llvm_stackmaps : {
        PROVIDE(__start_LLVM_STACKMAPS = .);
        *(.llvm_stackmaps)
    }
}

INSERT AFTER .text;
