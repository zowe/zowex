package org.zowe.zowex.ffm;

import java.lang.foreign.Arena;
import java.lang.foreign.Linker;
import java.lang.foreign.SymbolLookup;
import java.nio.file.Paths;

public class NativeLoader {
    private static final SymbolLookup LOOKUP;
    public static final Linker LINKER = Linker.nativeLinker();

    static {
        try {
            System.load(Paths.get("../bindings/build-out/libzowex_java.so").toAbsolutePath().toString());
        } catch (UnsatisfiedLinkError e) {
            System.err.println("Failed to load native library from ../bindings/build-out/libzowex_java.so");
            System.err.println("Trying to load from java.library.path instead...");
            System.loadLibrary("zowex_java");
        }
        LOOKUP = SymbolLookup.loaderLookup();
    }

    public static SymbolLookup getLookup() {
        return LOOKUP;
    }
}
