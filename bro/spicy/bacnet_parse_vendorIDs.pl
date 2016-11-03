#!/usr/bin/env perl
#
# Parser that downloads the current list of BACnet vendor ids from 
# http://www.bacnet.org/VendorID/BACnet%20Vendor%20IDs.htm and outputs
# it as a spicyc-list
#
# Sorry, the way this is done is probably a bit of overkill.

use 5.16.1;

use EV;
use Web::Scraper::LibXML;
use YADA;

YADA->new(
  	common_opts => {
		encoding => '',
		followlocation => 1,
		maxredirs => 5,
	}, http_response => 1, max => 4,
)->append([qw[
		http://www.bacnet.org/VendorID/BACnet%20Vendor%20IDs.htm
]] => sub {
	my ($self) = @_;

	if ( $self->has_error || !$self->response->is_success ) { 
		say "Error: ".Dumper($self);
		return;
	} 

	state $scraper = scraper {
		process '//body/table/tr', "members[]" => scraper {
      process 'td', 'tds[]' => 'TEXT';
    };
  };

	my $doc = $scraper->scrape(
		$self->response->decoded_content,
		$self->final_url,
	);
  
  for my $member (@{$doc->{'members'}}) {
    next unless defined($$member{'tds'});
    my ($id, $name) = @{$$member{'tds'}};
    $name =~ s/[^[:ascii:]]//g;
    $name =~ tr# -,&()[]:\.+/\\#__#;
    say "  $name = $id,";
  }

})->wait;
