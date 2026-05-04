package org.zowe.zowex.controllers;

import org.springframework.web.bind.annotation.*;
import org.zowe.zowex.ffm.ZusfBindings;

import java.util.Map;

@RestController
@RequestMapping("/zosmf/restfiles/fs")
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

    @PostMapping("/file")
    public Map<String, String> createUssFile(@RequestParam("path") String path,
                                             @RequestParam(value = "mode", defaultValue = "644") String mode) throws Exception {
        ZusfBindings.createUssFile(path, mode);
        return Map.of("message", "File created successfully");
    }

    @PostMapping("/dir")
    public Map<String, String> createUssDir(@RequestParam("path") String path,
                                            @RequestParam(value = "mode", defaultValue = "755") String mode) throws Exception {
        ZusfBindings.createUssDir(path, mode);
        return Map.of("message", "Directory created successfully");
    }

    @PutMapping("/move")
    public Map<String, String> moveUssFileOrDir(@RequestParam("source") String source,
                                                @RequestParam("destination") String destination) throws Exception {
        ZusfBindings.moveUssFileOrDir(source, destination);
        return Map.of("message", "Moved successfully");
    }

    @PutMapping("/chmod")
    public Map<String, String> chmodUssItem(@RequestParam("path") String path,
                                            @RequestParam("mode") String mode,
                                            @RequestParam(value = "recursive", defaultValue = "false") boolean recursive) throws Exception {
        ZusfBindings.chmodUssItem(path, mode, recursive);
        return Map.of("message", "Permissions changed successfully");
    }

    @PutMapping("/chown")
    public Map<String, String> chownUssItem(@RequestParam("path") String path,
                                            @RequestParam("owner") String owner,
                                            @RequestParam(value = "recursive", defaultValue = "false") boolean recursive) throws Exception {
        ZusfBindings.chownUssItem(path, owner, recursive);
        return Map.of("message", "Owner changed successfully");
    }

    @PutMapping("/chtag")
    public Map<String, String> chtagUssItem(@RequestParam("path") String path,
                                            @RequestParam("tag") String tag,
                                            @RequestParam(value = "recursive", defaultValue = "false") boolean recursive) throws Exception {
        ZusfBindings.chtagUssItem(path, tag, recursive);
        return Map.of("message", "Tag changed successfully");
    }

    @DeleteMapping("/**")
    public Map<String, String> deleteUssItem(@RequestParam("path") String path,
                                             @RequestParam(value = "recursive", defaultValue = "false") boolean recursive) throws Exception {
        ZusfBindings.deleteUssItem(path, recursive);
        return Map.of("message", "Deleted successfully");
    }
}
