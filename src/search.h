#pragma once

typedef 
struct _search_request
{
	const char *needle;
	int needle_size;
	int match_pos;
}
search_request;

void init_search_request(search_request *req, const char *needle);
int search(search_request *req, const char *data, int len);
