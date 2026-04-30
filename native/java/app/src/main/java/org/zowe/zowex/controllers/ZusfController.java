package org.zowe.zowex.controllers;

import org.springframework.web.bind.annotation.*;
import org.zowe.zowex.ffm.ZusfBindings;

import java.util.Map;

@RestController
@RequestMapping("/pythonservice/zosmf/restfiles/fs")
public class ZusfController {

    @GetMapping
    public String listUssDir(@RequestParam("path") String path,
                             @RequestParam(value = "all", defaultValue = "false") boolean allFiles,
                             @RequestParam(value = "long", defaultValue = "false") boolean longFormat) throws Exception {
        if (path == null || path.isEmpty()) {
            throw new IllegalArgumentException("path parameter is required");
        }
        // Returns the CSV or string formatted by the native C++ code
        return ZusfBindings.listUssDir(path, allFiles, longFormat, 1);
    }

    @GetMapping("/**") // wildcard for paths
    public String readUssFile(@RequestParam("path") String path,
                              @RequestParam(value = "encoding", required = false) String encoding) throws Exception {
        return ZusfBindings.readUssFile(path, encoding != null ? encoding : "");
    }

    @PutMapping("/**")
    public String writeUssFile(@RequestParam("path") String path,
                               @RequestBody String data,
                               @RequestParam(value = "encoding", required = false) String encoding) throws Exception {
        return ZusfBindings.writeUssFile(path, data, encoding != null ? encoding : "", "");
    }
}
