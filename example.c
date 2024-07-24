#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fb_api.h"

typedef struct fb_field
{
	unsigned field_type;
	unsigned offset;
	unsigned nullOffset;
	int scale;
	char name[252];
} fb_field_t;


void fill_field(fb_field_t* field, struct fb_MessageMetadata* meta, struct fb_Status* status, unsigned field_no)
{
	memset(field, 0, sizeof(fb_field_t));
	field->field_type = fb_MessageMetadata_getType(meta, status, field_no);
	field->offset = fb_MessageMetadata_getOffset(meta, status, field_no);
	field->nullOffset = fb_MessageMetadata_getNullOffset(meta, status, field_no);
	field->scale = fb_MessageMetadata_getScale(meta, status, field_no);	
	const char* name  = fb_MessageMetadata_getAlias(meta, status, field_no);
	
	memcpy(field->name, name, 252);
}




const char* db_uri = "inet://localhost:3050/employee";
const char* db_user = "SYSDBA";
const char* db_password = "masterkey";

const char* cursor_sql = "SELECT CAST(1.57 AS NUMERIC(38, 2)) AS N, CURRENT_TIMESTAMP AS TS FROM RDB$DATABASE";


int main (int argc, char** argv)
{
	printf("C API test\n");

	struct fb_Master* master = fb_get_master_interface();
	struct fb_Status* status = fb_Master_getStatus(master);
	struct fb_Provider* provider = fb_Master_getDispatcher(master);
	struct fb_Util* util = fb_Master_getUtilInterface(master);
	struct fb_Int128* int128u = NULL;

	struct fb_XpbBuilder* dpb_builder = NULL;
	unsigned dpb_length = 0;
	const unsigned char* dpb = NULL;

	struct fb_Attachment* att = NULL;

	struct fb_XpbBuilder* tpb_builder = NULL;
	unsigned tpb_length = 0;
	const unsigned char* tpb = NULL;

	struct fb_Transaction* tra = NULL;

	struct fb_Statement* stmt = NULL;

	struct fb_MessageMetadata* meta = NULL;

	unsigned column_count = 0;
	unsigned cursor_data_size = 0;

	struct fb_ResultSet* rs = NULL;

	unsigned char* cursor_data = NULL;

	do {
		int128u = fb_Util_getInt128(util, status);
		/* check error */
		if (fb_Status_getState(status)) {
			break;
		}

		/* build database parameters buffer */
		dpb_builder = fb_Util_getXpbBuilder(util, status, fb_XpbBuilder_DPB, NULL, 0);
		fb_XpbBuilder_insertString(dpb_builder, status, isc_dpb_user_name, db_user);
		fb_XpbBuilder_insertString(dpb_builder, status, isc_dpb_password, db_password);
		dpb_length = fb_XpbBuilder_getBufferLength(dpb_builder, status);
		dpb = fb_XpbBuilder_getBuffer(dpb_builder, status);
		/* check error */
		if (fb_Status_getState(status)) {
			break;
		}

		printf("Attache to database...\n");
		att = fb_Provider_attachDatabase(provider, status, db_uri, dpb_length, dpb);
		/* check error */
		if (fb_Status_getState(status)) {
			break;
		}

		/* build transaction parameters buffer */
		tpb_builder = fb_Util_getXpbBuilder(util, status, fb_XpbBuilder_TPB, NULL, 0);
		fb_XpbBuilder_insertTag(tpb_builder, status, isc_tpb_read_committed);
		fb_XpbBuilder_insertTag(tpb_builder, status, isc_tpb_read_consistency);
		fb_XpbBuilder_insertTag(tpb_builder, status, isc_tpb_wait);
		fb_XpbBuilder_insertTag(tpb_builder, status, isc_tpb_read);
		tpb_length = fb_XpbBuilder_getBufferLength(tpb_builder, status);
		tpb = fb_XpbBuilder_getBuffer(tpb_builder, status);
		/* check error */
		if (fb_Status_getState(status)) {
			break;
		}

		printf("Start transaction...\n");
		tra = fb_Attachment_startTransaction(att, status, tpb_length, tpb);
		/* check error */
		if (fb_Status_getState(status)) {
			break;
		}

		printf("Prepare statement...\n");
		stmt = fb_Attachment_prepare(att, status, tra, 0, cursor_sql, 3, fb_Statement_PREPARE_PREFETCH_METADATA);
		/* check error */
		if (fb_Status_getState(status)) {
			break;
		}

		/* get metadata */
		meta = fb_Statement_getOutputMetadata(stmt, status);
		column_count = fb_MessageMetadata_getCount(meta, status);
		cursor_data_size = fb_MessageMetadata_getMessageLength(meta, status);
		/* check error */
		if (fb_Status_getState(status)) {
			break;
		}

		printf("Open cursor...\n");
		rs = fb_Statement_openCursor(stmt, status, tra, NULL, NULL, meta, 0);
		/* check error */
		if (fb_Status_getState(status)) {
			break;
		}

		cursor_data = (unsigned char*)malloc(cursor_data_size);
		if (!cursor_data) {
			/* Error!!! */
			break;
		}

		while (fb_ResultSet_fetchNext(rs, status, cursor_data) == fb_Status_RESULT_OK) {
			/* check error */
			if (fb_Status_getState(status)) {
				break;
			}
			short nullFlag;
			const FB_I128* i128 = NULL;
			const ISC_TIMESTAMP_TZ* ts = NULL;
			unsigned year, month, day;
			unsigned hour, minute, second, fraction;

			for (unsigned i = 0; i < column_count; ++i) {
                fb_field_t field = {0};
				fill_field(&field, meta, status, i);
                if (fb_Status_getState(status)) {
					goto cleanup;
				}

                printf("%s: ", field.name);
				nullFlag = *(short*)(cursor_data + field.nullOffset);
				if (nullFlag == -1) {
					printf("<null>\t");
					continue;
				}

				switch (field.field_type) {
				case SQL_INT128:
				{
					i128 = (FB_I128*)(cursor_data + field.offset);
					char int128Buf[fb_Int128_STRING_SIZE] = {0};
					fb_Int128_toString(int128u, status, i128, field.scale, fb_Int128_STRING_SIZE, int128Buf);
					printf("%s", int128Buf);
				}
					break;
				case SQL_TIMESTAMP_TZ:
				{
					ts = (ISC_TIMESTAMP_TZ*)(cursor_data + field.offset);
					char tz_buffer[60] = { 0 };
					fb_Util_decodeTimeStampTz(util, status, ts,
						&year, &month, &day,
						&hour, &minute, &second, &fraction,
						sizeof(tz_buffer), tz_buffer);
					printf("%d-%d-%d %d:%d:%d %s", year, month, day, hour, minute, second, tz_buffer);
				}
				break;
				default:
				    printf("Unknown datatype");
					break;
				}

				/* check error */
				if (fb_Status_getState(status)) {
					goto cleanup;
				}

				printf("\t");
			}
			printf("\n");
		}

		/* check error */
		if (fb_Status_getState(status)) {
			break;
		}

		printf("Close cursor...\n");
		fb_ResultSet_close(rs, status);
		/* check error */
		if (fb_Status_getState(status)) {
			break;
		}
		rs = NULL;

		/* release metadata */
		fb_MessageMetadata_release(meta);
		meta = NULL;

		/* Free statement */
		fb_Statement_free(stmt, status);
		/* check error */
		if (fb_Status_getState(status)) {
			break;
		}
		stmt = NULL;

		printf("Commit transaction...\n");
		fb_Transaction_commit(tra, status);
		/* check error */
		if (fb_Status_getState(status)) {
			break;
		}
		tra = NULL;

		printf("Detach from database...\n");
		fb_Attachment_detach(att, status);
		/* check error */
		if (fb_Status_getState(status)) {
			break;
		}
		att = NULL;
	}
	while (0);

cleanup:

	printf("Clean\n");

	free(cursor_data);
	cursor_data = NULL;
	if (rs) {
		fb_ResultSet_release(rs);
		rs = NULL;
	}
	if (meta) {
		fb_MessageMetadata_release(meta);
		meta = NULL;
	}
	if (stmt) {
		fb_Statement_release(stmt);
		stmt = NULL;
	}
	if (tra) {
		fb_Transaction_release(tra);
		tra = NULL;
	}
	if (tpb_builder) {
		fb_XpbBuilder_dispose(tpb_builder);
		tpb_builder = NULL;
	}
	if (att) {
		fb_Attachment_release(att);
		att = NULL;
	}
	if (dpb_builder) {
		fb_XpbBuilder_dispose(dpb_builder);
		dpb_builder = NULL;
	}

	/* check error and print message */
	if (fb_Status_getState(status)) {
		char buf[256];
		fb_Util_formatStatus(util, buf, sizeof(buf), status);
		printf("Error: %s\n", buf);
		return 1;
	}
	

	return 0;
}