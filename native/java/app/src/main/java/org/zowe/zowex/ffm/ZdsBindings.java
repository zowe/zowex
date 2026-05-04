package org.zowe.zowex.ffm;

import java.lang.foreign.*;
import java.lang.invoke.MethodHandle;
import java.util.ArrayList;
import java.util.List;

public class ZdsBindings {

    private static final MethodHandle zds_c_list_data_sets;
    private static final MethodHandle zds_c_read_data_set;
    private static final MethodHandle zds_c_write_data_set;
    private static final MethodHandle zds_c_delete_data_set;
    private static final MethodHandle zds_c_free_list_response;
    private static final MethodHandle zds_c_free_string_response;
    private static final MethodHandle zds_c_free_basic_response;

    static {
        Linker linker = NativeLoader.LINKER;
        SymbolLookup lookup = NativeLoader.getLookup();

        zds_c_list_data_sets = linker.downcallHandle(
            lookup.find("zds_c_list_data_sets").orElseThrow(),
            FunctionDescriptor.of(ValueLayout.ADDRESS, ValueLayout.ADDRESS)
        );

        zds_c_read_data_set = linker.downcallHandle(
            lookup.find("zds_c_read_data_set").orElseThrow(),
            FunctionDescriptor.of(ValueLayout.ADDRESS, ValueLayout.ADDRESS, ValueLayout.ADDRESS)
        );

        zds_c_write_data_set = linker.downcallHandle(
            lookup.find("zds_c_write_data_set").orElseThrow(),
            FunctionDescriptor.of(ValueLayout.ADDRESS, ValueLayout.ADDRESS, ValueLayout.ADDRESS, ValueLayout.ADDRESS, ValueLayout.ADDRESS)
        );

        zds_c_delete_data_set = linker.downcallHandle(
            lookup.find("zds_c_delete_data_set").orElseThrow(),
            FunctionDescriptor.of(ValueLayout.ADDRESS, ValueLayout.ADDRESS)
        );

        zds_c_free_list_response = linker.downcallHandle(
            lookup.find("zds_c_free_list_response").orElseThrow(),
            FunctionDescriptor.ofVoid(ValueLayout.ADDRESS)
        );

        zds_c_free_string_response = linker.downcallHandle(
            lookup.find("zds_c_free_string_response").orElseThrow(),
            FunctionDescriptor.ofVoid(ValueLayout.ADDRESS)
        );

        zds_c_free_basic_response = linker.downcallHandle(
            lookup.find("zds_c_free_basic_response").orElseThrow(),
            FunctionDescriptor.ofVoid(ValueLayout.ADDRESS)
        );
    }

    public static class ZDSEntry {
        public String name;
        public String dsorg;
        public String volser;
        public String recfm;
        public boolean migrated;
    }

    public static List<ZDSEntry> listDataSets(String dsn) throws Exception {
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment dsnSeg = FfmUtils.allocateString(arena, dsn);
            MemorySegment responsePtr = (MemorySegment) zds_c_list_data_sets.invokeExact(dsnSeg);
            
            if (responsePtr.address() == 0) throw new RuntimeException("Null response from native library");
            
            responsePtr = responsePtr.reinterpret(24);

            MemorySegment errorMsgSeg = responsePtr.get(ValueLayout.ADDRESS, 16);
            String errorMsg = FfmUtils.readString(errorMsgSeg);
            if (errorMsg != null) {
                zds_c_free_list_response.invokeExact(responsePtr);
                throw new RuntimeException(errorMsg);
            }

            MemorySegment entriesPtr = responsePtr.get(ValueLayout.ADDRESS, 0);
            long count = responsePtr.get(ValueLayout.JAVA_LONG, 8);
            
            List<ZDSEntry> results = new ArrayList<>();
            if (count > 0 && entriesPtr.address() != 0) {
                // reinterpret entries array to read structs
                MemorySegment entriesArray = entriesPtr.reinterpret(count * 40); // 40 bytes per ZDSEntry_C
                for (long i = 0; i < count; i++) {
                    long offset = i * 40;
                    ZDSEntry entry = new ZDSEntry();
                    entry.name = FfmUtils.readString(entriesArray.get(ValueLayout.ADDRESS, offset));
                    entry.dsorg = FfmUtils.readString(entriesArray.get(ValueLayout.ADDRESS, offset + 8));
                    entry.volser = FfmUtils.readString(entriesArray.get(ValueLayout.ADDRESS, offset + 16));
                    entry.recfm = FfmUtils.readString(entriesArray.get(ValueLayout.ADDRESS, offset + 24));
                    entry.migrated = entriesArray.get(ValueLayout.JAVA_BOOLEAN, offset + 32);
                    results.add(entry);
                }
            }
            zds_c_free_list_response.invokeExact(responsePtr);
            return results;
        } catch (Throwable e) {
            if (e instanceof Exception) throw (Exception) e;
            throw new RuntimeException(e);
        }
    }

    public static String readDataSet(String dsn, String codepage) throws Exception {
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment dsnSeg = FfmUtils.allocateString(arena, dsn);
            MemorySegment cpSeg = FfmUtils.allocateString(arena, codepage);
            MemorySegment responsePtr = (MemorySegment) zds_c_read_data_set.invokeExact(dsnSeg, cpSeg);
            
            if (responsePtr.address() == 0) throw new RuntimeException("Null response from native library");
            
            responsePtr = responsePtr.reinterpret(16);

            MemorySegment errorMsgSeg = responsePtr.get(ValueLayout.ADDRESS, 8);
            String errorMsg = FfmUtils.readString(errorMsgSeg);
            if (errorMsg != null) {
                zds_c_free_string_response.invokeExact(responsePtr);
                throw new RuntimeException(errorMsg);
            }

            MemorySegment dataSeg = responsePtr.get(ValueLayout.ADDRESS, 0);
            String data = FfmUtils.readString(dataSeg);
            zds_c_free_string_response.invokeExact(responsePtr);
            return data;
        } catch (Throwable e) {
            if (e instanceof Exception) throw (Exception) e;
            throw new RuntimeException(e);
        }
    }

    public static String writeDataSet(String dsn, String data, String codepage, String etag) throws Exception {
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment dsnSeg = FfmUtils.allocateString(arena, dsn);
            MemorySegment dataSeg = FfmUtils.allocateString(arena, data);
            MemorySegment cpSeg = FfmUtils.allocateString(arena, codepage);
            MemorySegment etagSeg = FfmUtils.allocateString(arena, etag);

            MemorySegment responsePtr = (MemorySegment) zds_c_write_data_set.invokeExact(dsnSeg, dataSeg, cpSeg, etagSeg);
            
            if (responsePtr.address() == 0) throw new RuntimeException("Null response from native library");
            
            responsePtr = responsePtr.reinterpret(16);

            MemorySegment errorMsgSeg = responsePtr.get(ValueLayout.ADDRESS, 8);
            String errorMsg = FfmUtils.readString(errorMsgSeg);
            if (errorMsg != null) {
                zds_c_free_string_response.invokeExact(responsePtr);
                throw new RuntimeException(errorMsg);
            }

            MemorySegment outEtagSeg = responsePtr.get(ValueLayout.ADDRESS, 0);
            String outEtag = FfmUtils.readString(outEtagSeg);
            zds_c_free_string_response.invokeExact(responsePtr);
            return outEtag;
        } catch (Throwable e) {
            if (e instanceof Exception) throw (Exception) e;
            throw new RuntimeException(e);
        }
    }

    public static void deleteDataSet(String dsn) throws Exception {
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment dsnSeg = FfmUtils.allocateString(arena, dsn);
            MemorySegment responsePtr = (MemorySegment) zds_c_delete_data_set.invokeExact(dsnSeg);
            
            if (responsePtr.address() == 0) throw new RuntimeException("Null response from native library");
            
            responsePtr = responsePtr.reinterpret(8);

            MemorySegment errorMsgSeg = responsePtr.get(ValueLayout.ADDRESS, 0);
            String errorMsg = FfmUtils.readString(errorMsgSeg);
            zds_c_free_basic_response.invokeExact(responsePtr);
            
            if (errorMsg != null) {
                throw new RuntimeException(errorMsg);
            }
        } catch (Throwable e) {
            if (e instanceof Exception) throw (Exception) e;
            throw new RuntimeException(e);
        }
    }
}
