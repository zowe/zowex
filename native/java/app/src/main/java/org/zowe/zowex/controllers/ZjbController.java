package org.zowe.zowex.controllers;

import org.springframework.web.bind.annotation.*;
import org.zowe.zowex.ffm.ZjbBindings;
import org.zowe.zowex.ffm.ZjbBindings.ZJob;

import org.zowe.zowex.ffm.ZjbBindings.ZJobDD;

import java.util.List;
import java.util.Map;
import java.util.HashMap;

@RestController
@RequestMapping("/zosmf/restjobs/jobs")
public class ZjbController {

    @GetMapping
    public List<ZJob> listJobs(@RequestParam(value = "owner", defaultValue = "*") String owner,
                               @RequestParam(value = "prefix", defaultValue = "*") String prefix,
                               @RequestParam(value = "status", required = false) String status) throws Exception {
        return ZjbBindings.listJobsByOwner(owner, prefix, status != null ? status : "");
    }

    @GetMapping("/{jobid}")
    public ZJob getJobStatus(@PathVariable("jobid") String jobid) throws Exception {
        return ZjbBindings.getJobStatus(jobid);
    }

    @PutMapping
    public String submitJob(@RequestBody String jclContent) throws Exception {
        return ZjbBindings.submitJob(jclContent);
    }

    @GetMapping("/{jobid}/spool")
    public List<ZJobDD> listSpoolFiles(@PathVariable("jobid") String jobid) throws Exception {
        return ZjbBindings.listSpoolFiles(jobid);
    }

    @GetMapping("/{jobid}/spool/{key}")
    public String readSpoolFile(@PathVariable("jobid") String jobid, @PathVariable("key") int key) throws Exception {
        return ZjbBindings.readSpoolFile(jobid, key);
    }

    @GetMapping("/{jobid}/jcl")
    public String getJobJcl(@PathVariable("jobid") String jobid) throws Exception {
        return ZjbBindings.getJobJcl(jobid);
    }

    @DeleteMapping("/{jobid}")
    public Map<String, String> deleteJob(@PathVariable("jobid") String jobid) throws Exception {
        ZjbBindings.deleteJob(jobid);
        return Map.of("message", "Job deleted successfully");
    }
}
