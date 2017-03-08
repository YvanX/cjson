#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include "json.h"

int main()
{
	json obj;
	char *buf;

	const char *err;
	FILE *fp;
	size_t len;
	fp = fopen("test.json", "rb");
	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	buf = (char*)malloc(len + 1);
	fread(buf, 1, len, fp);
	fclose(fp);
	buf[len] = '\0';

	obj = json_Parse(buf, &err);
	free(buf);

	buf = json_Print(obj, 1);
	printf("%s", buf);
	json_Delete(obj);
	free(buf);

	int ints[5] = { 1, 2, 3, 4, 5};
	double nums[5] = { 1.1, 2.2, 3.3, 4.4, 5.5 };
	const char *strs[] = {
		"ÄãºÃ", "JSON", "Parser", "Yvanx"
	};
	json obj_nums, obj_strings;
	obj = json_MakeIntArray(ints, 5);
	obj_nums = json_MakeDoubleArray(nums, 5);
	obj_strings = json_MakeStringArray(strs, 4);
	json_AddItemToArray(obj, obj_nums);
	json_AddItemToArray(obj_nums, obj_strings);

	json sub_obj;
	sub_obj = json_Create(Type_Object);
	json_AddItemToObject(sub_obj, "action", json_CreateString("Test"));
	json_AddItemToObject(sub_obj, "list", json_MakeIntArray(ints, 3));
	json_UpdateItemInArray(obj_nums, 5, sub_obj);
	json tmp;
	tmp = json_DetachItemFromArray(obj_nums, 0);
	json_DeleteItemFromArray(obj_nums, 4);
	json_Delete(tmp);

	buf = json_Print(obj, 1);
	printf(buf);

	json_Delete(obj);
	free(buf);
	return 0;
}
