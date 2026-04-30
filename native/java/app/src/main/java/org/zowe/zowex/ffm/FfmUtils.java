package org.zowe.zowex.ffm;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;

public class FfmUtils {
    public static String readString(MemorySegment segment) {
        if (segment == null || segment.address() == 0) {
            return null;
        }
        return segment.getString(0);
    }
    
    public static MemorySegment allocateString(Arena arena, String str) {
        if (str == null) return MemorySegment.NULL;
        return arena.allocateFrom(str);
    }
}
