# Troubleshooting

## go: FSUM7351 not found

Ensure go is part of PATH

## Client connection error - Error: All configured authentication methods failed

Check that your username and password are correct.<br/>
For private keys, confirm that you can ssh into the LPAR/zVDT using it.

## FSUM9383 Configuration file `/etc/startup.mk' not found

You should be able to find the `startup.mk` file in `/samples`

- `cp /samples/startup.mk /etc/startup.mk` <br/>
  _source:_ https://www.ibm.com/support/pages/fsum9383-configuration-file-etcstartupmk-not-found

## Building zut.o - FSUM3221 xlc: Cannot spawn program /usr/lpp/cbclib/xlc/exe/ccndrvr

> **Note:** Since the project uses `ibm-clang++64` for C++ compilation, this issue should only occur for Metal C builds which use `xlc`.

One workaround is to add `CBC.SCCNCMP` to your system LINKLIST concatenation. Below is an example of doing this via SYSVIEW commands.

:warning: These commands could ruin your system if the linklist is corrupted. Do not modify the linklist unless you know what you are doing. :warning:

```
linklist
linkdef zowex from current
linklist zowex
add CBC.SCCNCMP
linkact zowex
set asid 1
linkupd *
```

Note 1: You may need to run `linkact zowex` after an IPL if your linklist has been reset.<br/>
Note 2: Depending on your z/OS configuration, you may need to replace `*` with your mask character. For example, `linkact zowex =`

## Downloading Go dependencies: tls: failed to verify certificate: x509: certificate signed by unknown authority

Update your `config.yaml` to include this property:

- `goBuildEnv: 'GOINSECURE="*" GOPROXY=direct GIT_SSL_NO_VERIFY=true'  # Allow fetching Go dependencies`

## CEE3561S ... was not found in DLL CRTEQCXE

```
CEE3561S External function _ZNSt5__1_e13__hash_memoryEPKvm was not found in DLL CRTEQCXE
```

`zowex` links dynamically against the C++ runtime (libc++) that IBM ships **inside Language Environment**, in the DLL
`CRTEQCXE`. This message means the system running the binary does not export a function the binary needs, so z/OS
cannot load it. It happens before `main()`, which is why `zowex` cannot report anything useful itself.

The deciding factor is the **Language Environment maintenance (PTF) level** of the system running the binary, not just
its z/OS release — a system at a supported release can still be missing the required service. Three things contribute:

| Lever | Effect |
|---|---|
| Open XL C/C++ level used to build | Determines which libc++ symbols the binary references. Dominant factor. Open XL 2.1 is supported on z/OS 2.4/2.5/3.1; **Open XL 2.2 requires z/OS 3.1+** and references a newer libc++. |
| LE maintenance level on the target | Determines which libc++ symbols the target exports. |
| `-mzos-target` (`ZosTarget`) | Only the LE **system-header** API level, via `__TARGET_LIB__`. Makes no link-step change and does **not** restrict libc++, so it will not resolve this message on its own. |

### If you are running a released build

Ask your system programmer to confirm Language Environment maintenance is applied:

- **z/OS 2.5** — the LE C++ runtime service for Open XL C/C++ (the `PH45516` APAR family).
- **z/OS 3.1** — the IBM Z Distribution for Zowe program directory lists `PH53938`, `PH60056`, `PH62468` and `PH68179`
  as required for Language Environment.

### If you are building it yourself

Build with the Open XL level that matches your oldest target system. `.github/workflows/zos-build.yml` pins
`/usr/lpp/IBM/cnw/v2r1/openxl/bin` (Open XL 2.1) for exactly this reason; put the same on your `PATH` via
`preBuildCmd` in `config.yaml`.

Then check what the binary actually needs:

```sh
npm run z:check:compat
```

This compares the imported symbols against `native/c/compat/`. If the check reports a symbol that is unavailable on the
minimum supported release, the fix is to stop using it — not to refresh the baseline. See
[`native/c/compat/README.md`](../native/c/compat/README.md).

To make this fail at **bind** time on your build host rather than at load time on someone else's system, copy the LE
C++ side decks from a system at your minimum supported release and point `LDFLAGS` at those copies (see the commented
examples in `config.example.yaml`).

## Every compile fails with: error: unknown argument '-mzos-target=...'

Your Open XL C/C++ level does not support the option. Clang treats an unknown `-m` argument as an error, not a warning,
so this breaks every compile. Disable the option in `config.yaml`:

```yaml
    zosTarget: none
```

Equivalently, `make -DZosTarget=none` or `ZOWE_NATIVE_ZOS_TARGET=none npm run z:build`. Note Open XL 2.x rejects target
levels older than `zosv2r4`.
