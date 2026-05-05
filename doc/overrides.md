# Overriding Behavior

There is limited support for overriding behaviors and attributes of `zowex`.

## Registered Products

`zowex` may register as a [running product](https://www.ibm.com/docs/en/zos/3.2.0?topic=services-register-service-ifaedreg) on the system. The registration of a product is associated with a vendor.  While "Zowe" is not a vendor, any vendor may instruct or supply a `usage.txt` contained in an `overrides` directory adjacent to the `zowex` binary. The format of the file is `keyword=value`.  Supported keywords are:

- `vendor=`, e.g. `vendor=some mainframe vendor`

The location of the directory can also be specified via an environmental variable `ZOWEX_OVERRIDES_DIR`.

## Features

Valid features of register products are:

- `ZRS VSCE`
- `ZRS CLI` (zowe CLI plugin)
- `ZRS MCP`
- `CLI` (zowex CLI)
