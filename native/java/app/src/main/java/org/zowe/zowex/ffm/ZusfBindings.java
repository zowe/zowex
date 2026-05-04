package org.zowe.zowex.ffm;

import java.lang.foreign.*;
import java.lang.invoke.MethodHandle;

public class ZusfBindings {

    private static final MethodHandle zusf_c_list_uss_dir;
    private static final MethodHandle zusf_c_read_uss_file;
    private static final MethodHandle zusf_c_write_uss_file;
    private static final MethodHandle zusf_c_create_uss_file;
    private static final MethodHandle zusf_c_create_uss_dir;
    private static final MethodHandle zusf_c_move_uss_file_or_dir;
    private static final MethodHandle zusf_c_chmod_uss_item;
    private static final MethodHandle zusf_c_delete_uss_item;
    private static final MethodHandle zusf_c_chown_uss_item;
    private static final MethodHandle zusf_c_chtag_uss_item;
    private static final MethodHandle zusf_c_free_string_response;
    private static final MethodHandle zusf_c_free_basic_response;

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

        zusf_c_create_uss_file = linker.downcallHandle(
            lookup.find("zusf_c_create_uss_file").orElseThrow(),
            FunctionDescriptor.of(ValueLayout.ADDRESS, ValueLayout.ADDRESS, ValueLayout.ADDRESS)
        );

        zusf_c_create_uss_dir = linker.downcallHandle(
            lookup.find("zusf_c_create_uss_dir").orElseThrow(),
            FunctionDescriptor.of(ValueLayout.ADDRESS, ValueLayout.ADDRESS, ValueLayout.ADDRESS)
        );

        zusf_c_move_uss_file_or_dir = linker.downcallHandle(
            lookup.find("zusf_c_move_uss_file_or_dir").orElseThrow(),
            FunctionDescriptor.of(ValueLayout.ADDRESS, ValueLayout.ADDRESS, ValueLayout.ADDRESS)
        );

        zusf_c_chmod_uss_item = linker.downcallHandle(
            lookup.find("zusf_c_chmod_uss_item").orElseThrow(),
            FunctionDescriptor.of(ValueLayout.ADDRESS, ValueLayout.ADDRESS, ValueLayout.ADDRESS, ValueLayout.JAVA_BOOLEAN)
        );

        zusf_c_delete_uss_item = linker.downcallHandle(
            lookup.find("zusf_c_delete_uss_item").orElseThrow(),
            FunctionDescriptor.of(ValueLayout.ADDRESS, ValueLayout.ADDRESS, ValueLayout.JAVA_BOOLEAN)
        );

        zusf_c_chown_uss_item = linker.downcallHandle(
            lookup.find("zusf_c_chown_uss_item").orElseThrow(),
            FunctionDescriptor.of(ValueLayout.ADDRESS, ValueLayout.ADDRESS, ValueLayout.ADDRESS, ValueLayout.JAVA_BOOLEAN)
        );

        zusf_c_chtag_uss_item = linker.downcallHandle(
            lookup.find("zusf_c_chtag_uss_item").orElseThrow(),
            FunctionDescriptor.of(ValueLayout.ADDRESS, ValueLayout.ADDRESS, ValueLayout.ADDRESS, ValueLayout.JAVA_BOOLEAN)
        );

        zusf_c_free_string_response = linker.downcallHandle(
            lookup.find("zusf_c_free_string_response").orElseThrow(),
            FunctionDescriptor.ofVoid(ValueLayout.ADDRESS)
        );

        zusf_c_free_basic_response = linker.downcallHandle(
            lookup.find("zusf_c_free_basic_response").orElseThrow(),
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

    private static void handleBasicResponse(MemorySegment responsePtr) throws Exception {
        if (responsePtr.address() == 0) throw new RuntimeException("Null response from native library");
        responsePtr = responsePtr.reinterpret(8);
        MemorySegment errorMsgSeg = responsePtr.get(ValueLayout.ADDRESS, 0);
        String errorMsg = FfmUtils.readString(errorMsgSeg);
        try {
            zusf_c_free_basic_response.invokeExact(responsePtr);
        } catch (Throwable e) {
            if (e instanceof Exception) throw (Exception) e;
            throw new RuntimeException(e);
        }
        if (errorMsg != null) {
            throw new RuntimeException(errorMsg);
        }
    }

    public static void createUssFile(String file, String mode) throws Exception {
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment fileSeg = FfmUtils.allocateString(arena, file);
            MemorySegment modeSeg = FfmUtils.allocateString(arena, mode);
            MemorySegment responsePtr = (MemorySegment) zusf_c_create_uss_file.invokeExact(fileSeg, modeSeg);
            handleBasicResponse(responsePtr);
        } catch (Throwable e) {
            if (e instanceof Exception) throw (Exception) e;
            throw new RuntimeException(e);
        }
    }

    public static void createUssDir(String file, String mode) throws Exception {
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment fileSeg = FfmUtils.allocateString(arena, file);
            MemorySegment modeSeg = FfmUtils.allocateString(arena, mode);
            MemorySegment responsePtr = (MemorySegment) zusf_c_create_uss_dir.invokeExact(fileSeg, modeSeg);
            handleBasicResponse(responsePtr);
        } catch (Throwable e) {
            if (e instanceof Exception) throw (Exception) e;
            throw new RuntimeException(e);
        }
    }

    public static void moveUssFileOrDir(String source, String destination) throws Exception {
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment srcSeg = FfmUtils.allocateString(arena, source);
            MemorySegment destSeg = FfmUtils.allocateString(arena, destination);
            MemorySegment responsePtr = (MemorySegment) zusf_c_move_uss_file_or_dir.invokeExact(srcSeg, destSeg);
            handleBasicResponse(responsePtr);
        } catch (Throwable e) {
            if (e instanceof Exception) throw (Exception) e;
            throw new RuntimeException(e);
        }
    }

    public static void chmodUssItem(String file, String mode, boolean recursive) throws Exception {
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment fileSeg = FfmUtils.allocateString(arena, file);
            MemorySegment modeSeg = FfmUtils.allocateString(arena, mode);
            MemorySegment responsePtr = (MemorySegment) zusf_c_chmod_uss_item.invokeExact(fileSeg, modeSeg, recursive);
            handleBasicResponse(responsePtr);
        } catch (Throwable e) {
            if (e instanceof Exception) throw (Exception) e;
            throw new RuntimeException(e);
        }
    }

    public static void deleteUssItem(String file, boolean recursive) throws Exception {
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment fileSeg = FfmUtils.allocateString(arena, file);
            MemorySegment responsePtr = (MemorySegment) zusf_c_delete_uss_item.invokeExact(fileSeg, recursive);
            handleBasicResponse(responsePtr);
        } catch (Throwable e) {
            if (e instanceof Exception) throw (Exception) e;
            throw new RuntimeException(e);
        }
    }

    public static void chownUssItem(String file, String owner, boolean recursive) throws Exception {
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment fileSeg = FfmUtils.allocateString(arena, file);
            MemorySegment ownerSeg = FfmUtils.allocateString(arena, owner);
            MemorySegment responsePtr = (MemorySegment) zusf_c_chown_uss_item.invokeExact(fileSeg, ownerSeg, recursive);
            handleBasicResponse(responsePtr);
        } catch (Throwable e) {
            if (e instanceof Exception) throw (Exception) e;
            throw new RuntimeException(e);
        }
    }

    public static void chtagUssItem(String file, String tag, boolean recursive) throws Exception {
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment fileSeg = FfmUtils.allocateString(arena, file);
            MemorySegment tagSeg = FfmUtils.allocateString(arena, tag);
            MemorySegment responsePtr = (MemorySegment) zusf_c_chtag_uss_item.invokeExact(fileSeg, tagSeg, recursive);
            handleBasicResponse(responsePtr);
        } catch (Throwable e) {
            if (e instanceof Exception) throw (Exception) e;
            throw new RuntimeException(e);
        }
    }
}
