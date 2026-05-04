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
    private static final MethodHandle zds_c_create_data_set;
    private static final MethodHandle zds_c_create_member;
    private static final MethodHandle zds_c_list_members;
    private static final MethodHandle zds_c_free_list_response;
    private static final MethodHandle zds_c_free_string_response;
    private static final MethodHandle zds_c_free_basic_response;
    private static final MethodHandle zds_c_free_mem_list_response;

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

        zds_c_create_data_set = linker.downcallHandle(
            lookup.find("zds_c_create_data_set").orElseThrow(),
            FunctionDescriptor.of(ValueLayout.ADDRESS, ValueLayout.ADDRESS, ValueLayout.ADDRESS)
        );

        zds_c_create_member = linker.downcallHandle(
            lookup.find("zds_c_create_member").orElseThrow(),
            FunctionDescriptor.of(ValueLayout.ADDRESS, ValueLayout.ADDRESS)
        );

        zds_c_list_members = linker.downcallHandle(
            lookup.find("zds_c_list_members").orElseThrow(),
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

        zds_c_free_mem_list_response = linker.downcallHandle(
            lookup.find("zds_c_free_mem_list_response").orElseThrow(),
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

    public static class ZDSMem {
        public String name;
    }

    public static class DSAttributes {
        public String alcunit;
        public int blksize;
        public int dirblk;
        public String dsorg;
        public int primary;
        public String recfm;
        public int lrecl;
        public String dataclass;
        public String unit;
        public String dsntype;
        public String mgntclass;
        public String dsname;
        public int avgblk;
        public int secondary;
        public int size;
        public String storclass;
        public String vol;
    }

    private static void handleBasicResponse(MemorySegment responsePtr) throws Exception {
        if (responsePtr.address() == 0) throw new RuntimeException("Null response from native library");
        responsePtr = responsePtr.reinterpret(8);
        MemorySegment errorMsgSeg = responsePtr.get(ValueLayout.ADDRESS, 0);
        String errorMsg = FfmUtils.readString(errorMsgSeg);
        try {
            zds_c_free_basic_response.invokeExact(responsePtr);
        } catch (Throwable e) {
            if (e instanceof Exception) throw (Exception) e;
            throw new RuntimeException(e);
        }
        if (errorMsg != null) {
            throw new RuntimeException(errorMsg);
        }
    }

    public static void createDataSet(String dsn, DSAttributes attrs) throws Exception {
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment dsnSeg = FfmUtils.allocateString(arena, dsn);
            MemorySegment attrsSeg = MemorySegment.NULL;
            
            if (attrs != null) {
                // Struct layout:
                // 0: alcunit (ptr)
                // 8: blksize (int)
                // 12: dirblk (int)
                // 16: dsorg (ptr)
                // 24: primary (int)
                // 28: padding (4)
                // 32: recfm (ptr)
                // 40: lrecl (int)
                // 44: padding (4)
                // 48: dataclass (ptr)
                // 56: unit (ptr)
                // 64: dsntype (ptr)
                // 72: mgntclass (ptr)
                // 80: dsname (ptr)
                // 88: avgblk (int)
                // 92: secondary (int)
                // 96: size (int)
                // 100: padding (4)
                // 104: storclass (ptr)
                // 112: vol (ptr)
                // Total size: 120 bytes
                attrsSeg = arena.allocate(120);
                attrsSeg.set(ValueLayout.ADDRESS, 0, FfmUtils.allocateString(arena, attrs.alcunit));
                attrsSeg.set(ValueLayout.JAVA_INT, 8, attrs.blksize);
                attrsSeg.set(ValueLayout.JAVA_INT, 12, attrs.dirblk);
                attrsSeg.set(ValueLayout.ADDRESS, 16, FfmUtils.allocateString(arena, attrs.dsorg));
                attrsSeg.set(ValueLayout.JAVA_INT, 24, attrs.primary);
                attrsSeg.set(ValueLayout.ADDRESS, 32, FfmUtils.allocateString(arena, attrs.recfm));
                attrsSeg.set(ValueLayout.JAVA_INT, 40, attrs.lrecl);
                attrsSeg.set(ValueLayout.ADDRESS, 48, FfmUtils.allocateString(arena, attrs.dataclass));
                attrsSeg.set(ValueLayout.ADDRESS, 56, FfmUtils.allocateString(arena, attrs.unit));
                attrsSeg.set(ValueLayout.ADDRESS, 64, FfmUtils.allocateString(arena, attrs.dsntype));
                attrsSeg.set(ValueLayout.ADDRESS, 72, FfmUtils.allocateString(arena, attrs.mgntclass));
                attrsSeg.set(ValueLayout.ADDRESS, 80, FfmUtils.allocateString(arena, attrs.dsname));
                attrsSeg.set(ValueLayout.JAVA_INT, 88, attrs.avgblk);
                attrsSeg.set(ValueLayout.JAVA_INT, 92, attrs.secondary);
                attrsSeg.set(ValueLayout.JAVA_INT, 96, attrs.size);
                attrsSeg.set(ValueLayout.ADDRESS, 104, FfmUtils.allocateString(arena, attrs.storclass));
                attrsSeg.set(ValueLayout.ADDRESS, 112, FfmUtils.allocateString(arena, attrs.vol));
            }

            MemorySegment responsePtr = (MemorySegment) zds_c_create_data_set.invokeExact(dsnSeg, attrsSeg);
            handleBasicResponse(responsePtr);
        } catch (Throwable e) {
            if (e instanceof Exception) throw (Exception) e;
            throw new RuntimeException(e);
        }
    }

    public static void createMember(String dsn) throws Exception {
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment dsnSeg = FfmUtils.allocateString(arena, dsn);
            MemorySegment responsePtr = (MemorySegment) zds_c_create_member.invokeExact(dsnSeg);
            handleBasicResponse(responsePtr);
        } catch (Throwable e) {
            if (e instanceof Exception) throw (Exception) e;
            throw new RuntimeException(e);
        }
    }

    public static List<ZDSMem> listMembers(String dsn) throws Exception {
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment dsnSeg = FfmUtils.allocateString(arena, dsn);
            MemorySegment responsePtr = (MemorySegment) zds_c_list_members.invokeExact(dsnSeg);
            
            if (responsePtr.address() == 0) throw new RuntimeException("Null response from native library");
            
            responsePtr = responsePtr.reinterpret(24);

            MemorySegment errorMsgSeg = responsePtr.get(ValueLayout.ADDRESS, 16);
            String errorMsg = FfmUtils.readString(errorMsgSeg);
            if (errorMsg != null) {
                zds_c_free_mem_list_response.invokeExact(responsePtr);
                throw new RuntimeException(errorMsg);
            }

            MemorySegment membersPtr = responsePtr.get(ValueLayout.ADDRESS, 0);
            long count = responsePtr.get(ValueLayout.JAVA_LONG, 8);
            
            List<ZDSMem> results = new ArrayList<>();
            if (count > 0 && membersPtr.address() != 0) {
                MemorySegment membersArray = membersPtr.reinterpret(count * 8); // 8 bytes per ZDSMem_C (just a pointer)
                for (long i = 0; i < count; i++) {
                    ZDSMem mem = new ZDSMem();
                    mem.name = FfmUtils.readString(membersArray.get(ValueLayout.ADDRESS, i * 8));
                    results.add(mem);
                }
            }
            zds_c_free_mem_list_response.invokeExact(responsePtr);
            return results;
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
