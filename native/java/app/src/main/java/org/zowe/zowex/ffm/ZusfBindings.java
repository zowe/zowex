package org.zowe.zowex.ffm;

import java.lang.foreign.*;
import java.lang.invoke.MethodHandle;

public class ZusfBindings {

    private static final MethodHandle zusf_c_list_uss_dir;
    private static final MethodHandle zusf_c_read_uss_file;
    private static final MethodHandle zusf_c_write_uss_file;
    private static final MethodHandle zusf_c_free_string_response;

    static {
        Linker linker = NativeLoader.LINKER;
        SymbolLookup lookup = NativeLoader.getLookup();

        zusf_c_list_uss_dir = linker.downcallHandle(
            lookup.find("zusf_c_list_uss_dir").orElseThrow(),
            FunctionDescriptor.of(ValueLayout.ADDRESS, ValueLayout.ADDRESS, ValueLayout.ADDRESS)
        );

        zusf_c_read_uss_file = linker.downcallHandle(
            lookup.find("zusf_c_read_uss_file").orElseThrow(),
            FunctionDescriptor.of(ValueLayout.ADDRESS, ValueLayout.ADDRESS, ValueLayout.ADDRESS)
        );

        zusf_c_write_uss_file = linker.downcallHandle(
            lookup.find("zusf_c_write_uss_file").orElseThrow(),
            FunctionDescriptor.of(ValueLayout.ADDRESS, ValueLayout.ADDRESS, ValueLayout.ADDRESS, ValueLayout.ADDRESS, ValueLayout.ADDRESS)
        );

        zusf_c_free_string_response = linker.downcallHandle(
            lookup.find("zusf_c_free_string_response").orElseThrow(),
            FunctionDescriptor.ofVoid(ValueLayout.ADDRESS)
        );
    }

    public static String listUssDir(String path, boolean allFiles, boolean longFormat, int maxDepth) throws Exception {
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment pathSeg = FfmUtils.allocateString(arena, path);
            
            // Allocate ZUSFListOptions_C: bool(1) + bool(1) + int(4) -> 8 bytes total aligned?
            // Actually struct is bool, bool, padding(2), int(4) -> 8 bytes.
            MemorySegment optionsSeg = arena.allocate(8);
            optionsSeg.set(ValueLayout.JAVA_BOOLEAN, 0, allFiles);
            optionsSeg.set(ValueLayout.JAVA_BOOLEAN, 1, longFormat);
            optionsSeg.set(ValueLayout.JAVA_INT, 4, maxDepth);

            MemorySegment responsePtr = (MemorySegment) zusf_c_list_uss_dir.invokeExact(pathSeg, optionsSeg);
            
            if (responsePtr.address() == 0) throw new RuntimeException("Null response from native library");
            
            responsePtr = responsePtr.reinterpret(16);

            MemorySegment errorMsgSeg = responsePtr.get(ValueLayout.ADDRESS, 8);
            String errorMsg = FfmUtils.readString(errorMsgSeg);
            if (errorMsg != null) {
                zusf_c_free_string_response.invokeExact(responsePtr);
                throw new RuntimeException(errorMsg);
            }

            MemorySegment dataSeg = responsePtr.get(ValueLayout.ADDRESS, 0);
            String data = FfmUtils.readString(dataSeg);
            zusf_c_free_string_response.invokeExact(responsePtr);
            return data;
        } catch (Throwable e) {
            if (e instanceof Exception) throw (Exception) e;
            throw new RuntimeException(e);
        }
    }

    public static String readUssFile(String file, String codepage) throws Exception {
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment fileSeg = FfmUtils.allocateString(arena, file);
            MemorySegment cpSeg = FfmUtils.allocateString(arena, codepage);
            MemorySegment responsePtr = (MemorySegment) zusf_c_read_uss_file.invokeExact(fileSeg, cpSeg);
            
            if (responsePtr.address() == 0) throw new RuntimeException("Null response from native library");
            
            responsePtr = responsePtr.reinterpret(16);

            MemorySegment errorMsgSeg = responsePtr.get(ValueLayout.ADDRESS, 8);
            String errorMsg = FfmUtils.readString(errorMsgSeg);
            if (errorMsg != null) {
                zusf_c_free_string_response.invokeExact(responsePtr);
                throw new RuntimeException(errorMsg);
            }

            MemorySegment dataSeg = responsePtr.get(ValueLayout.ADDRESS, 0);
            String data = FfmUtils.readString(dataSeg);
            zusf_c_free_string_response.invokeExact(responsePtr);
            return data;
        } catch (Throwable e) {
            if (e instanceof Exception) throw (Exception) e;
            throw new RuntimeException(e);
        }
    }

    public static String writeUssFile(String file, String data, String codepage, String etag) throws Exception {
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment fileSeg = FfmUtils.allocateString(arena, file);
            MemorySegment dataSeg = FfmUtils.allocateString(arena, data);
            MemorySegment cpSeg = FfmUtils.allocateString(arena, codepage);
            MemorySegment etagSeg = FfmUtils.allocateString(arena, etag);

            MemorySegment responsePtr = (MemorySegment) zusf_c_write_uss_file.invokeExact(fileSeg, dataSeg, cpSeg, etagSeg);
            
            if (responsePtr.address() == 0) throw new RuntimeException("Null response from native library");
            
            responsePtr = responsePtr.reinterpret(16);

            MemorySegment errorMsgSeg = responsePtr.get(ValueLayout.ADDRESS, 8);
            String errorMsg = FfmUtils.readString(errorMsgSeg);
            if (errorMsg != null) {
                zusf_c_free_string_response.invokeExact(responsePtr);
                throw new RuntimeException(errorMsg);
            }

            MemorySegment outDataSeg = responsePtr.get(ValueLayout.ADDRESS, 0);
            String outEtag = FfmUtils.readString(outDataSeg);
            zusf_c_free_string_response.invokeExact(responsePtr);
            return outEtag;
        } catch (Throwable e) {
            if (e instanceof Exception) throw (Exception) e;
            throw new RuntimeException(e);
        }
    }
}
