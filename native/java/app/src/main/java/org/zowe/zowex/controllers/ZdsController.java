package org.zowe.zowex.controllers;

import org.springframework.web.bind.annotation.*;
import org.zowe.zowex.ffm.ZdsBindings;
import org.zowe.zowex.ffm.ZdsBindings.ZDSEntry;

import org.zowe.zowex.ffm.ZdsBindings.ZDSMem;

import java.util.List;
import java.util.Map;
import java.util.HashMap;

@RestController
@RequestMapping("/zosmf/restfiles/ds")
public class ZdsController {

    @GetMapping
    public Map<String, Object> listDataSets(@RequestParam("dslevel") String dslevel) throws Exception {
        if (dslevel == null || dslevel.isEmpty()) {
            throw new IllegalArgumentException("dslevel parameter is required");
        }
        String dsn = dslevel + ".**";
        List<ZDSEntry> entries = ZdsBindings.listDataSets(dsn);
        
        Map<String, Object> response = new HashMap<>();
        response.put("items", entries);
        response.put("returnedRows", entries.size());
        return response;
    }

    @GetMapping("/{dsn}")
    public String readDataSet(@PathVariable("dsn") String dsn, @RequestParam(value = "encoding", required = false) String encoding) throws Exception {
        return ZdsBindings.readDataSet(dsn, encoding != null ? encoding : "");
    }

    @PostMapping("/{dsn}")
    public Map<String, String> createDataSet(@PathVariable("dsn") String dsn, @RequestBody(required = false) ZdsBindings.DSAttributes attrs) throws Exception {
        ZdsBindings.createDataSet(dsn, attrs);
        return Map.of("message", "Data set created successfully");
    }

    @PostMapping("/{dsn}/member/{member}")
    public Map<String, String> createMember(@PathVariable("dsn") String dsn, @PathVariable("member") String member) throws Exception {
        String fullDsn = dsn + "(" + member + ")";
        ZdsBindings.createMember(fullDsn);
        return Map.of("message", "Member created successfully");
    }

    @GetMapping("/{dsn}/members")
    public Map<String, Object> listMembers(@PathVariable("dsn") String dsn) throws Exception {
        List<ZDSMem> members = ZdsBindings.listMembers(dsn);
        Map<String, Object> response = new HashMap<>();
        response.put("items", members);
        response.put("returnedRows", members.size());
        return response;
    }

    @PutMapping("/{dsn}")
    public String writeDataSet(@PathVariable("dsn") String dsn, @RequestBody String data, @RequestParam(value = "encoding", required = false) String encoding) throws Exception {
        return ZdsBindings.writeDataSet(dsn, data, encoding != null ? encoding : "", "");
    }

    @DeleteMapping("/{dsn}")
    public Map<String, String> deleteDataSet(@PathVariable("dsn") String dsn) throws Exception {
        ZdsBindings.deleteDataSet(dsn);
        return Map.of("message", "Deleted successfully");
    }
}
