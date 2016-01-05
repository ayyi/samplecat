$(function(){
	setTimeout(function(){
		//console.log("doing table..." + $.fn.dataTable);
	}, 2000);

	$('.btn').click(function(){
		location.href = "/search";
	});
});

var dt;
function do_table(){
	dt = $('table#samples').dataTable({
		paging: false
		//order: [[0, "desc"]]
	}).api();
}
