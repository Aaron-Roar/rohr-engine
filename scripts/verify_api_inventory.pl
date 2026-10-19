#!/usr/bin/env perl
# Copyright 2026 Aaron Rohrer
# SPDX-License-Identifier: LGPL-3.0-only

use strict;
use warnings;
use FindBin;
use File::Find qw(find);
use File::Spec;
use Cwd qw(abs_path);

# Audit all installed first-party headers, including undocumented declarations,
# callable macros and facade aliases. This is an inventory check, not a C parser
# or an ABI/signature compatibility checker. A second prototype scan guards
# against silently dropping declarations unsupported by the primary scan.
my $root = abs_path($ARGV[0] // "$FindBin::Bin/..");
die "usage: $0 [repository-root]\n" if @ARGV > 1 || !defined $root;
my @errors;
sub fail { push @errors, $_[0]; }
sub read_file {
    my ($path) = @_;
    open my $file, '<', $path or die "open $path: $!\n";
    local $/;
    return <$file>;
}
sub compare_names {
    my ($label, $actual, $listed) = @_;
    for my $name (sort keys %$actual) {
        fail("$label: missing $name") unless exists $listed->{$name};
    }
    for my $name (sort keys %$listed) {
        fail("$label: stale $name") unless exists $actual->{$name};
    }
}
sub source_without_comments {
    my ($source) = @_;
    $source =~ s/\\\r?\n//g;
    # Preserve quoted literals and newlines while removing comments.
    $source =~ s{("(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*')|(/\*.*?\*/|//[^\n]*)}{
        defined $1 ? $1 : ($2 =~ s/[^\n]/ /gr)
    }gse;
    return $source;
}

my (%headers, %configuration_types);
find({no_chdir => 1, wanted => sub {
    return unless -f $_ && /\.h$/;
    my $name = File::Spec->abs2rel($_, "$root/include");
    $name =~ s{\\}{/}g;
    my $source = source_without_comments(read_file($_));
    my (%entries, %functions);
    while ($source =~ /^\s*#\s*define[ \t]+(\w+)(\([^\n]*?\))?([^\n]*)/mg) {
        my ($macro, $args, $body) = ($1, $2, $3);
        if (defined $args || ($macro =~ /^rohr_/ && $body =~ /^\s*[a-z]\w*\s*$/)) {
            fail("$name: duplicate declaration $macro") if $entries{$macro}++;
        }
    }
    $source =~ s/^\s*#[^\n]*//mg;
    while ($source =~ /^\s*((?:[A-Za-z_]\w*[ \t\n*]+)+)([A-Za-z_]\w*)\s*\(([^;{}]*?)\)\s*;/mg) {
        my ($return_type, $function) = ($1, $2);
        next if $return_type =~ /\b(?:typedef|return)\b/;
        fail("$name: duplicate declaration $function") if $entries{$function}++;
        $functions{$function} = 1;
    }
    my %independent;
    my $prototype_source = $source;
    $prototype_source =~ s/\btypedef\b[^;]*;//sg;
    while ($prototype_source =~ /\b([a-z][a-zA-Z0-9_]*)\s*\([^;{}]*\)\s*;/g) {
        $independent{$1} = 1 unless $1 =~ /^(?:void|bool|int|float|double)$/;
    }
    compare_names("$name prototype parser", \%independent, \%functions);
    while ($source =~ /\b([A-Z]\w*(?:Config|Descriptor))\b/g) {
        $configuration_types{$1} = 1;
    }
    $headers{$name} = \%entries;
}}, "$root/include");
fail('no public headers found') unless keys %headers;

my $document_path = "$root/docs/description_api_contract.md";
my $document = read_file($document_path);
my (%document_headers, %classification_totals);
my $total = 0;
while ($document =~ m{<details>\s*<summary>([\w./-]+\.h): (\d+) entries</summary>(.*?)</details>}sg) {
    my ($header, $count, $body) = ($1, $2, $3);
    fail("duplicate header section $header") if $document_headers{$header}++;
    my (%listed, $classification);
    for my $line (split /\n/, $body) {
        # Match the inventory's bold category heading without depending on its
        # human-readable label or UTF-8 dash spelling.
        if ($line =~ /^\*\*(\w+)\s/) { $classification = $1; }
        next unless $line =~ /^- /;
        while ($line =~ /`(\w+)`/g) {
            my $name = $1;
            fail("$header: duplicate inventory entry $name") if $listed{$name}++;
            if (!defined $classification) { fail("$header: unclassified $name"); }
            else { $classification_totals{$classification}++; }
        }
    }
    my $listed_count = 0;
    $listed_count += $_ for values %listed;
    fail("$header: summary says $count entries, lists $listed_count") if $count != $listed_count;
    compare_names($header, $headers{$header} // {}, \%listed);
    $total += $listed_count;
}
compare_names('header sections', \%headers, \%document_headers);
my ($declared_total) = $document =~ /The current inventory contains \*\*(\d+)\*\*/;
fail("inventory total must be $total") if !defined $declared_total || $declared_total != $total;
my %table_totals;
while ($document =~ /^\| (\w+): [^|]*\|[^|]*\|[^|]*\|\s*(\d+)\s*\|/mg) {
    my ($category, $count) = ($1, $2);
    fail("duplicate classification total $category") if exists $table_totals{$category};
    $table_totals{$category} = $count;
}
for my $category (sort keys %classification_totals) {
    fail("$category: classification total must be $classification_totals{$category}")
        if !exists $table_totals{$category} || $table_totals{$category} != $classification_totals{$category};
}
for my $category (sort keys %table_totals) {
    fail("$category: unused classification total")
        if !exists $classification_totals{$category} && $table_totals{$category} != 0;
}
my %document_names = map { $_ => 1 } ($document =~ /`(\w+)`/g);
for my $name (sort keys %configuration_types) {
    fail("configuration type $name has no documented migration/exemption") unless $document_names{$name};
}
# Check relative file links in the inventory. Fragment contents are deliberately
# outside this audit; historical API names in explanatory prose remain valid.
while ($document =~ /\]\(([^)]+)\)/g) {
    my $target = $1;
    next if $target =~ m{^[a-z][a-z0-9+.-]*://|^#}i;
    $target =~ s/#.*//;
    fail("broken inventory link $target") unless -e "$root/docs/$target";
}
if (@errors) {
    print STDERR "API inventory audit failed:\n", map { "  $_\n" } @errors;
    exit 1;
}
printf "PASS: %d installed headers; %d classified API entries; independent prototype comparison\n", scalar(keys %headers), $total;
printf "PASS: %d configuration names documented; header/category totals and local links agree\n", scalar(keys %configuration_types);
