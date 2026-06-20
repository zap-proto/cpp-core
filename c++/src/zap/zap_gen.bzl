"""Support rule to invoke zap compiler."""

zap_provider = provider("Zap Provider", fields = {
    "includes": "includes for this target (transitive)",
    "inputs": "src + data for the target",
    "src_prefix": "src_prefix of the target",
})

def _workspace_path(label, path):
    if label.workspace_root == "":
        return path
    return label.workspace_root + "/" + path

def _zap_gen_impl(ctx):
    label = ctx.label
    src_prefix = _workspace_path(label, ctx.attr.src_prefix) if ctx.attr.src_prefix != "" else ""
    includes = []

    inputs = ctx.files.srcs + ctx.files.data
    for dep_target in ctx.attr.deps:
        includes += dep_target[zap_provider].includes
        inputs += dep_target[zap_provider].inputs

    if src_prefix != "":
        includes.append(src_prefix)

    system_include = ctx.files._zap_system[0].dirname.removesuffix("/zap")

    gen_dir = ctx.var["GENDIR"]
    out_dir = gen_dir
    if src_prefix != "":
        out_dir = out_dir + "/" + src_prefix

    cc_out = "-o%s:%s" % (ctx.executable.zapc_plugin.path, out_dir)
    args = ctx.actions.args()
    args.add_all(["compile", "--verbose", cc_out])
    args.add_all(["-I" + inc for inc in includes])
    args.add_all(["-I", system_include])

    if src_prefix == "":
        # guess src_prefix for generated files
        for src in ctx.files.srcs:
            if src.path.startswith(gen_dir):
                src_prefix = gen_dir
                break

    if src_prefix != "":
        args.add_all(["--src-prefix", src_prefix])

    args.add_all([s for s in ctx.files.srcs])

    ctx.actions.run(
        inputs = inputs + ctx.files.zapc_plugin + ctx.files._zapc_zap + ctx.files._zap_system,
        outputs = ctx.outputs.outs,
        executable = ctx.executable._zapc,
        arguments = [args],
        mnemonic = "GenZap",
    )

    return [
        zap_provider(
            includes = includes,
            inputs = inputs,
            src_prefix = src_prefix,
        ),
    ]

zap_gen = rule(
    attrs = {
        "srcs": attr.label_list(allow_files = True),
        "deps": attr.label_list(providers = [zap_provider]),
        "data": attr.label_list(allow_files = True),
        "outs": attr.output_list(),
        "src_prefix": attr.string(),
        "zapc_plugin": attr.label(executable = True, allow_single_file = True, cfg = "exec", mandatory = True),
        "_zapc": attr.label(executable = True, allow_single_file = True, cfg = "exec", default = "@zap-cpp//src/zap:zap_tool"),
        "_zapc_zap": attr.label(executable = True, allow_single_file = True, cfg = "exec", default = "@zap-cpp//src/zap:zapc-zap"),
        "_zap_system": attr.label(default = "@zap-cpp//src/zap:zap_system_library"),
    },
    output_to_genfiles = True,
    implementation = _zap_gen_impl,
)
