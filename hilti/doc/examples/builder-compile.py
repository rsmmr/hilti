# Turn the HILTI module into LLVM code.
(success, llvm_module) = mbuilder.codegen()

if not success:
	print "code generation failed"
	sys.exit(1)

# We can now do something with the LLVM module, like write it out as bitcode.
llvm_module.to_bitcode(open("hello-world.bc", "w"))
