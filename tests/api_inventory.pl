#!/usr/bin/env perl
# Copyright 2026 Aaron Rohrer
# SPDX-License-Identifier: LGPL-3.0-only

use strict;
use warnings;
use FindBin;
use File::Temp qw(tempdir);
use File::Path qw(make_path);
use IPC::Open3 qw(open3);
use Symbol qw(gensym);

my $root = tempdir(CLEANUP => 1);
make_path("$root/include", "$root/docs");
my $header = <<'HEADER';
#ifndef EXAMPLE_H
#define EXAMPLE_H
/* int commented_out(void); */
typedef struct ExampleConfig { int value; } ExampleConfig;
typedef void (*ExampleCallback)(void *context);
typedef size_t (*ExampleSizeCallback)(void *context);
int example_create(int value);
const char *example_get(void);
extern bool example_check(
    ExampleCallback callback,
    void *context);
#define EXAMPLE_CALL(value) example_create(value)
#define EXAMPLE_MORE(value) \
    example_create(value)
#define rohr_example_create example_create
#define EXAMPLE_LIMIT (16)
#define EXAMPLE_URL "https://example.invalid/*literal*/"
#endif
HEADER
my $document = <<'DOCUMENT';
# Test inventory

`ExampleConfig` is an intentional exemption.
The current inventory contains **6** classified declaration/macro entries.

| Classification | Meaning | Owning goal | Entries |
| --- | --- | --- | --- |
| C: Compliant | Description interface | — | 6 |

<details>
<summary>example.h: 6 entries</summary>

Source: [example.h](../include/example.h).

**C — Description interface**

- `example_create`, `example_get`, `example_check`.
- `EXAMPLE_CALL`, `EXAMPLE_MORE`, `rohr_example_create`.

</details>
DOCUMENT
sub write_file {
    my ($path, $value) = @_;
    open my $file, '>', $path or die "write $path: $!\n";
    print $file $value;
    close $file or die "close $path: $!\n";
}
sub check {
    my ($name, $source, $inventory, $diagnostic) = @_;
    write_file("$root/include/example.h", $source);
    write_file("$root/docs/description_api_contract.md", $inventory);
    my $error = gensym;
    my $pid = open3(undef, my $output, $error, $^X,
        "$FindBin::Bin/../scripts/verify_api_inventory.pl", $root);
    my $stdout = do { local $/; <$output> } // '';
    my $stderr = do { local $/; <$error> } // '';
    waitpid($pid, 0);
    my $status = $?;
    my $expected = defined $diagnostic ? 256 : 0;
    die "$name: unexpected status $status\n$stdout$stderr"
        if $status != $expected || (defined $diagnostic && index($stderr, $diagnostic) < 0);
    print "PASS: $name\n";
}
check('valid declarations, callbacks, multiline macros, and aliases', $header, $document, undef);
check('undocumented declaration', $header . "\nvoid new_api(void);\n", $document, 'missing new_api');
check('removed declaration', $header =~ s/int example_create\(int value\);//r, $document, 'stale example_create');
check('undocumented alias', $header . "\n#define rohr_new_api example_create\n", $document, 'missing rohr_new_api');
check('duplicate classification', $header, $document =~ s/`example_get`/`example_create`/r, 'duplicate inventory entry example_create');
check('unclassified declaration', $header, $document =~ s/\*\*C [^\n]*//r, 'unclassified example_create');
check('incorrect header count', $header, $document =~ s/example.h: 6/example.h: 7/r, 'summary says 7');
check('incorrect global count', $header, $document =~ s/contains \*\*6/contains **7/r, 'inventory total must be 6');
check('incorrect category count', $header, $document =~ s/\| 6 \|/| 7 |/r, 'classification total must be 6');
check('unclassified configuration', $header, $document =~ s/`ExampleConfig`/`OldConfig`/r, 'configuration type ExampleConfig');
check('broken link', $header, $document =~ s{\(../include/example.h\)}{(../include/missing.h)}r, 'broken inventory link');
check('missing header section', $header, $document =~ s/<details>.*?<\/details>//sr, 'header sections: missing example.h');
make_path("$root/include/nested");
write_file("$root/include/nested/new.h", "void nested_api(void);\n");
check('new installed subdirectory header', $header, $document, 'header sections: missing nested/new.h');
