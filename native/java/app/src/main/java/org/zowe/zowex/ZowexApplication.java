package org.zowe.zowex;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.zowe.apiml.enable.EnableApiDiscovery;

@SpringBootApplication
@EnableApiDiscovery
public class ZowexApplication {
    public static void main(String[] args) {
        SpringApplication.run(ZowexApplication.class, args);
    }
}
