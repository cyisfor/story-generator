struct <?cS name ?>_stmts {
	<?C
		for(i=0;i<nstmts;++i) {
			?>sqlite3_stmt* <?cS stmts[i].name?>;
	<?C
			}
	?>
		};

void <?cS name ?>_init() {
	<?C
		for(i=0;i<nstmts;++i) {
			?><?cS name ?>_stmts.<?cS stmts[i].name?> = db_prepare(LITLEN("<?C output_sql();?>"));
		}
}	
