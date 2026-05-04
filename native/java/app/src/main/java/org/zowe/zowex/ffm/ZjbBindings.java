package org.zowe.zowex.ffm;

import java.lang.foreign.*;
import java.lang.invoke.MethodHandle;
import java.util.ArrayList;
import java.util.List;

public class ZjbBindings {

    private static final MethodHandle zjb_c_list_jobs_by_owner;
    private static final MethodHandle zjb_c_get_job_status;
    private static final MethodHandle zjb_c_submit_job;
    private static final MethodHandle zjb_c_list_spool_files;
    private static final MethodHandle zjb_c_read_spool_file;
    private static final MethodHandle zjb_c_get_job_jcl;
    private static final MethodHandle zjb_c_delete_job;
    private static final MethodHandle zjb_c_free_job_list_response;
    private static final MethodHandle zjb_c_free_job_response;
    private static final MethodHandle zjb_c_free_string_response;
    private static final MethodHandle zjb_c_free_job_dd_list_response;
    private static final MethodHandle zjb_c_free_basic_response;

    static {
        Linker linker = NativeLoader.LINKER;
        SymbolLookup lookup = NativeLoader.getLookup();

        zjb_c_list_jobs_by_owner = linker.downcallHandle(
            lookup.find("zjb_c_list_jobs_by_owner").orElseThrow(),
            FunctionDescriptor.of(ValueLayout.ADDRESS, ValueLayout.ADDRESS, ValueLayout.ADDRESS, ValueLayout.ADDRESS)
        );

        zjb_c_get_job_status = linker.downcallHandle(
            lookup.find("zjb_c_get_job_status").orElseThrow(),
            FunctionDescriptor.of(ValueLayout.ADDRESS, ValueLayout.ADDRESS)
        );

        zjb_c_submit_job = linker.downcallHandle(
            lookup.find("zjb_c_submit_job").orElseThrow(),
            FunctionDescriptor.of(ValueLayout.ADDRESS, ValueLayout.ADDRESS)
        );

        zjb_c_list_spool_files = linker.downcallHandle(
            lookup.find("zjb_c_list_spool_files").orElseThrow(),
            FunctionDescriptor.of(ValueLayout.ADDRESS, ValueLayout.ADDRESS)
        );

        zjb_c_read_spool_file = linker.downcallHandle(
            lookup.find("zjb_c_read_spool_file").orElseThrow(),
            FunctionDescriptor.of(ValueLayout.ADDRESS, ValueLayout.ADDRESS, ValueLayout.JAVA_INT)
        );

        zjb_c_get_job_jcl = linker.downcallHandle(
            lookup.find("zjb_c_get_job_jcl").orElseThrow(),
            FunctionDescriptor.of(ValueLayout.ADDRESS, ValueLayout.ADDRESS)
        );

        zjb_c_delete_job = linker.downcallHandle(
            lookup.find("zjb_c_delete_job").orElseThrow(),
            FunctionDescriptor.of(ValueLayout.ADDRESS, ValueLayout.ADDRESS)
        );

        zjb_c_free_job_list_response = linker.downcallHandle(
            lookup.find("zjb_c_free_job_list_response").orElseThrow(),
            FunctionDescriptor.ofVoid(ValueLayout.ADDRESS)
        );

        zjb_c_free_job_response = linker.downcallHandle(
            lookup.find("zjb_c_free_job_response").orElseThrow(),
            FunctionDescriptor.ofVoid(ValueLayout.ADDRESS)
        );

        zjb_c_free_string_response = linker.downcallHandle(
            lookup.find("zjb_c_free_string_response").orElseThrow(),
            FunctionDescriptor.ofVoid(ValueLayout.ADDRESS)
        );

        zjb_c_free_job_dd_list_response = linker.downcallHandle(
            lookup.find("zjb_c_free_job_dd_list_response").orElseThrow(),
            FunctionDescriptor.ofVoid(ValueLayout.ADDRESS)
        );

        zjb_c_free_basic_response = linker.downcallHandle(
            lookup.find("zjb_c_free_basic_response").orElseThrow(),
            FunctionDescriptor.ofVoid(ValueLayout.ADDRESS)
        );
    }

    public static class ZJob {
        public String jobname;
        public String jobid;
        public String owner;
        public String status;
        public String fullStatus;
        public String retcode;
        public String correlator;
    }

    private static ZJob readZJobStruct(MemorySegment segment, long offset) {
        ZJob job = new ZJob();
        job.jobname = FfmUtils.readString(segment.get(ValueLayout.ADDRESS, offset));
        job.jobid = FfmUtils.readString(segment.get(ValueLayout.ADDRESS, offset + 8));
        job.owner = FfmUtils.readString(segment.get(ValueLayout.ADDRESS, offset + 16));
        job.status = FfmUtils.readString(segment.get(ValueLayout.ADDRESS, offset + 24));
        job.fullStatus = FfmUtils.readString(segment.get(ValueLayout.ADDRESS, offset + 32));
        job.retcode = FfmUtils.readString(segment.get(ValueLayout.ADDRESS, offset + 40));
        job.correlator = FfmUtils.readString(segment.get(ValueLayout.ADDRESS, offset + 48));
        return job;
    }

    public static List<ZJob> listJobsByOwner(String ownerName, String prefix, String status) throws Exception {
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment ownerSeg = FfmUtils.allocateString(arena, ownerName);
            MemorySegment prefixSeg = FfmUtils.allocateString(arena, prefix);
            MemorySegment statusSeg = FfmUtils.allocateString(arena, status);

            MemorySegment responsePtr = (MemorySegment) zjb_c_list_jobs_by_owner.invokeExact(ownerSeg, prefixSeg, statusSeg);
            
            if (responsePtr.address() == 0) throw new RuntimeException("Null response from native library");
            
            responsePtr = responsePtr.reinterpret(24);

            MemorySegment errorMsgSeg = responsePtr.get(ValueLayout.ADDRESS, 16);
            String errorMsg = FfmUtils.readString(errorMsgSeg);
            if (errorMsg != null) {
                zjb_c_free_job_list_response.invokeExact(responsePtr);
                throw new RuntimeException(errorMsg);
            }

            MemorySegment jobsPtr = responsePtr.get(ValueLayout.ADDRESS, 0);
            long count = responsePtr.get(ValueLayout.JAVA_LONG, 8);
            
            List<ZJob> results = new ArrayList<>();
            if (count > 0 && jobsPtr.address() != 0) {
                MemorySegment jobsArray = jobsPtr.reinterpret(count * 56); // 7 pointers * 8 bytes = 56 bytes
                for (long i = 0; i < count; i++) {
                    results.add(readZJobStruct(jobsArray, i * 56));
                }
            }
            zjb_c_free_job_list_response.invokeExact(responsePtr);
            return results;
        } catch (Throwable e) {
            if (e instanceof Exception) throw (Exception) e;
            throw new RuntimeException(e);
        }
    }

    public static ZJob getJobStatus(String jobid) throws Exception {
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment jobidSeg = FfmUtils.allocateString(arena, jobid);
            MemorySegment responsePtr = (MemorySegment) zjb_c_get_job_status.invokeExact(jobidSeg);
            
            if (responsePtr.address() == 0) throw new RuntimeException("Null response from native library");
            
            responsePtr = responsePtr.reinterpret(16);

            MemorySegment errorMsgSeg = responsePtr.get(ValueLayout.ADDRESS, 8);
            String errorMsg = FfmUtils.readString(errorMsgSeg);
            if (errorMsg != null) {
                zjb_c_free_job_response.invokeExact(responsePtr);
                throw new RuntimeException(errorMsg);
            }

            MemorySegment jobPtr = responsePtr.get(ValueLayout.ADDRESS, 0);
            ZJob job = null;
            if (jobPtr.address() != 0) {
                job = readZJobStruct(jobPtr.reinterpret(56), 0);
            }
            zjb_c_free_job_response.invokeExact(responsePtr);
            return job;
        } catch (Throwable e) {
            if (e instanceof Exception) throw (Exception) e;
            throw new RuntimeException(e);
        }
    }

    public static class ZJobDD {
        public String jobid;
        public String ddn;
        public String dsn;
        public String stepname;
        public String procstep;
        public int key;
    }

    private static void handleBasicResponse(MemorySegment responsePtr) throws Exception {
        if (responsePtr.address() == 0) throw new RuntimeException("Null response from native library");
        responsePtr = responsePtr.reinterpret(8);
        MemorySegment errorMsgSeg = responsePtr.get(ValueLayout.ADDRESS, 0);
        String errorMsg = FfmUtils.readString(errorMsgSeg);
        try {
            zjb_c_free_basic_response.invokeExact(responsePtr);
        } catch (Throwable e) {
            if (e instanceof Exception) throw (Exception) e;
            throw new RuntimeException(e);
        }
        if (errorMsg != null) {
            throw new RuntimeException(errorMsg);
        }
    }

    public static List<ZJobDD> listSpoolFiles(String jobid) throws Exception {
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment jobidSeg = FfmUtils.allocateString(arena, jobid);
            MemorySegment responsePtr = (MemorySegment) zjb_c_list_spool_files.invokeExact(jobidSeg);
            
            if (responsePtr.address() == 0) throw new RuntimeException("Null response from native library");
            
            responsePtr = responsePtr.reinterpret(24);

            MemorySegment errorMsgSeg = responsePtr.get(ValueLayout.ADDRESS, 16);
            String errorMsg = FfmUtils.readString(errorMsgSeg);
            if (errorMsg != null) {
                zjb_c_free_job_dd_list_response.invokeExact(responsePtr);
                throw new RuntimeException(errorMsg);
            }

            MemorySegment ddsPtr = responsePtr.get(ValueLayout.ADDRESS, 0);
            long count = responsePtr.get(ValueLayout.JAVA_LONG, 8);
            
            List<ZJobDD> results = new ArrayList<>();
            if (count > 0 && ddsPtr.address() != 0) {
                MemorySegment ddsArray = ddsPtr.reinterpret(count * 48); // 5 pointers + 1 int + padding = 48 bytes
                for (long i = 0; i < count; i++) {
                    long offset = i * 48;
                    ZJobDD dd = new ZJobDD();
                    dd.jobid = FfmUtils.readString(ddsArray.get(ValueLayout.ADDRESS, offset));
                    dd.ddn = FfmUtils.readString(ddsArray.get(ValueLayout.ADDRESS, offset + 8));
                    dd.dsn = FfmUtils.readString(ddsArray.get(ValueLayout.ADDRESS, offset + 16));
                    dd.stepname = FfmUtils.readString(ddsArray.get(ValueLayout.ADDRESS, offset + 24));
                    dd.procstep = FfmUtils.readString(ddsArray.get(ValueLayout.ADDRESS, offset + 32));
                    dd.key = ddsArray.get(ValueLayout.JAVA_INT, offset + 40);
                    results.add(dd);
                }
            }
            zjb_c_free_job_dd_list_response.invokeExact(responsePtr);
            return results;
        } catch (Throwable e) {
            if (e instanceof Exception) throw (Exception) e;
            throw new RuntimeException(e);
        }
    }

    public static String readSpoolFile(String jobid, int key) throws Exception {
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment jobidSeg = FfmUtils.allocateString(arena, jobid);
            MemorySegment responsePtr = (MemorySegment) zjb_c_read_spool_file.invokeExact(jobidSeg, key);
            
            if (responsePtr.address() == 0) throw new RuntimeException("Null response from native library");
            
            responsePtr = responsePtr.reinterpret(16);

            MemorySegment errorMsgSeg = responsePtr.get(ValueLayout.ADDRESS, 8);
            String errorMsg = FfmUtils.readString(errorMsgSeg);
            if (errorMsg != null) {
                zjb_c_free_string_response.invokeExact(responsePtr);
                throw new RuntimeException(errorMsg);
            }

            MemorySegment dataSeg = responsePtr.get(ValueLayout.ADDRESS, 0);
            String data = FfmUtils.readString(dataSeg);
            zjb_c_free_string_response.invokeExact(responsePtr);
            return data;
        } catch (Throwable e) {
            if (e instanceof Exception) throw (Exception) e;
            throw new RuntimeException(e);
        }
    }

    public static String getJobJcl(String jobid) throws Exception {
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment jobidSeg = FfmUtils.allocateString(arena, jobid);
            MemorySegment responsePtr = (MemorySegment) zjb_c_get_job_jcl.invokeExact(jobidSeg);
            
            if (responsePtr.address() == 0) throw new RuntimeException("Null response from native library");
            
            responsePtr = responsePtr.reinterpret(16);

            MemorySegment errorMsgSeg = responsePtr.get(ValueLayout.ADDRESS, 8);
            String errorMsg = FfmUtils.readString(errorMsgSeg);
            if (errorMsg != null) {
                zjb_c_free_string_response.invokeExact(responsePtr);
                throw new RuntimeException(errorMsg);
            }

            MemorySegment dataSeg = responsePtr.get(ValueLayout.ADDRESS, 0);
            String data = FfmUtils.readString(dataSeg);
            zjb_c_free_string_response.invokeExact(responsePtr);
            return data;
        } catch (Throwable e) {
            if (e instanceof Exception) throw (Exception) e;
            throw new RuntimeException(e);
        }
    }

    public static void deleteJob(String jobid) throws Exception {
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment jobidSeg = FfmUtils.allocateString(arena, jobid);
            MemorySegment responsePtr = (MemorySegment) zjb_c_delete_job.invokeExact(jobidSeg);
            handleBasicResponse(responsePtr);
        } catch (Throwable e) {
            if (e instanceof Exception) throw (Exception) e;
            throw new RuntimeException(e);
        }
    }

    public static String submitJob(String jclContent) throws Exception {
        try (Arena arena = Arena.ofConfined()) {
            MemorySegment jclSeg = FfmUtils.allocateString(arena, jclContent);
            MemorySegment responsePtr = (MemorySegment) zjb_c_submit_job.invokeExact(jclSeg);
            
            if (responsePtr.address() == 0) throw new RuntimeException("Null response from native library");
            
            responsePtr = responsePtr.reinterpret(16);

            MemorySegment errorMsgSeg = responsePtr.get(ValueLayout.ADDRESS, 8);
            String errorMsg = FfmUtils.readString(errorMsgSeg);
            if (errorMsg != null) {
                zjb_c_free_string_response.invokeExact(responsePtr);
                throw new RuntimeException(errorMsg);
            }

            MemorySegment jobidSeg = responsePtr.get(ValueLayout.ADDRESS, 0);
            String jobid = FfmUtils.readString(jobidSeg);
            zjb_c_free_string_response.invokeExact(responsePtr);
            return jobid;
        } catch (Throwable e) {
            if (e instanceof Exception) throw (Exception) e;
            throw new RuntimeException(e);
        }
    }
}
