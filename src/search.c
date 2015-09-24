#include "search.h"

void init_search_request(search_request *req, const char *needle)
{
	req->match_pos = 0;
	req->needle = needle;
}

int search(search_request *req, const char *data, int len)
{
	int i;
	for(i = 0; i < len; ++i)
	{
		if(req->needle[req->match_pos] == data[i])
		{
			++req->match_pos;
			if(req->match_pos == req->needle_size)
			{
				req->match_pos = 0;
				return i;
			}
		}
		else
		{
			req->match_pos = 0;
		}
	}
	return 0;
}
