#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "firebird/fb_c_api.h"

typedef struct fb_field
{
	unsigned field_type;
	unsigned offset;
	unsigned nullOffset;
	int scale;
	char name[252];
} fb_field_t;


void fill_field(fb_field_t* field, struct IMessageMetadata* meta, struct IStatus* status, unsigned field_no)
{
	memset(field, 0, sizeof(fb_field_t));
	field->field_type = IMessageMetadata_getType(meta, status, field_no);
	field->offset = IMessageMetadata_getOffset(meta, status, field_no);
	field->nullOffset = IMessageMetadata_getNullOffset(meta, status, field_no);
	field->scale = IMessageMetadata_getScale(meta, status, field_no);	
	const char* name  = IMessageMetadata_getAlias(meta, status, field_no);
	
	memcpy(field->name, name, 252);
}




const char* db_uri = "inet://localhost:3050/employee";
const char* db_user = "SYSDBA";
const char* db_password = "masterkey";

const char* cursor_sql = "SELECT CAST(1.57 AS NUMERIC(38, 2)) AS N, CURRENT_TIMESTAMP AS TS FROM RDB$DATABASE";


int main (int argc, char** argv)
{
	printf("C API test\n");

	struct IMaster* master = fb_get_master_interface();
	struct IStatus* status = IMaster_getStatus(master);
	struct IProvider* provider = IMaster_getDispatcher(master);
	struct IUtil* util = IMaster_getUtilInterface(master);
	struct IInt128* int128u = NULL;

	struct IXpbBuilder* dpb_builder = NULL;
	unsigned dpb_length = 0;
	const unsigned char* dpb = NULL;

	struct IAttachment* att = NULL;

	struct IXpbBuilder* tpb_builder = NULL;
	unsigned tpb_length = 0;
	const unsigned char* tpb = NULL;

	struct ITransaction* tra = NULL;

	struct IStatement* stmt = NULL;

	struct IMessageMetadata* meta = NULL;

	unsigned column_count = 0;
	unsigned cursor_data_size = 0;

	struct IResultSet* rs = NULL;

	unsigned char* cursor_data = NULL;
    unsigned client_version = 0;

	do {
        client_version = IUtil_getClientVersion(util);
        printf("Client Version: %d; major: %d, minor: %d", client_version, (client_version >> 8) & 0xFF, client_version & 0xFF);
        printf("\n");

		int128u = IUtil_getInt128(util, status);
		/* check error */
		if (IStatus_getState(status)) {
			break;
		}

		/* build database parameters buffer */
		dpb_builder = IUtil_getXpbBuilder(util, status, IXpbBuilder_DPB, NULL, 0);
		IXpbBuilder_insertString(dpb_builder, status, isc_dpb_user_name, db_user);
		IXpbBuilder_insertString(dpb_builder, status, isc_dpb_password, db_password);
		dpb_length = IXpbBuilder_getBufferLength(dpb_builder, status);
		dpb = IXpbBuilder_getBuffer(dpb_builder, status);
		/* check error */
		if (IStatus_getState(status)) {
			break;
		}

		printf("Attache to database...\n");
		att = IProvider_attachDatabase(provider, status, db_uri, dpb_length, dpb);
		/* check error */
		if (IStatus_getState(status)) {
			break;
		}

		/* build transaction parameters buffer */
		tpb_builder = IUtil_getXpbBuilder(util, status, IXpbBuilder_TPB, NULL, 0);
		IXpbBuilder_insertTag(tpb_builder, status, isc_tpb_read_committed);
		IXpbBuilder_insertTag(tpb_builder, status, isc_tpb_read_consistency);
		IXpbBuilder_insertTag(tpb_builder, status, isc_tpb_wait);
		IXpbBuilder_insertTag(tpb_builder, status, isc_tpb_read);
		tpb_length = IXpbBuilder_getBufferLength(tpb_builder, status);
		tpb = IXpbBuilder_getBuffer(tpb_builder, status);
		/* check error */
		if (IStatus_getState(status)) {
			break;
		}

		printf("Start transaction...\n");
		tra = IAttachment_startTransaction(att, status, tpb_length, tpb);
		/* check error */
		if (IStatus_getState(status)) {
			break;
		}

		printf("Prepare statement...\n");
		stmt = IAttachment_prepare(att, status, tra, 0, cursor_sql, 3, IStatement_PREPARE_PREFETCH_METADATA);
		/* check error */
		if (IStatus_getState(status)) {
			break;
		}

		/* get metadata */
		meta = IStatement_getOutputMetadata(stmt, status);
		column_count = IMessageMetadata_getCount(meta, status);
		cursor_data_size = IMessageMetadata_getMessageLength(meta, status);
		/* check error */
		if (IStatus_getState(status)) {
			break;
		}

		printf("Open cursor...\n");
		rs = IStatement_openCursor(stmt, status, tra, NULL, NULL, meta, 0);
		/* check error */
		if (IStatus_getState(status)) {
			break;
		}

		cursor_data = (unsigned char*)malloc(cursor_data_size);
		if (!cursor_data) {
			/* Error!!! */
			break;
		}

		while (IResultSet_fetchNext(rs, status, cursor_data) == IStatus_RESULT_OK) {
			/* check error */
			if (IStatus_getState(status)) {
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
                if (IStatus_getState(status)) {
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
					char int128Buf[IInt128_STRING_SIZE] = {0};
					IInt128_toString(int128u, status, i128, field.scale, IInt128_STRING_SIZE, int128Buf);
					printf("%s", int128Buf);
				}
					break;
				case SQL_TIMESTAMP_TZ:
				{
					ts = (ISC_TIMESTAMP_TZ*)(cursor_data + field.offset);
					char tz_buffer[60] = { 0 };
					IUtil_decodeTimeStampTz(util, status, ts,
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
				if (IStatus_getState(status)) {
					goto cleanup;
				}

				printf("\t");
			}
			printf("\n");
		}

		/* check error */
		if (IStatus_getState(status)) {
			break;
		}

		printf("Close cursor...\n");
		IResultSet_close(rs, status);
		/* check error */
		if (IStatus_getState(status)) {
			break;
		}
		rs = NULL;

		/* release metadata */
		IMessageMetadata_release(meta);
		meta = NULL;

		/* Free statement */
		IStatement_free(stmt, status);
		/* check error */
		if (IStatus_getState(status)) {
			break;
		}
		stmt = NULL;

		printf("Commit transaction...\n");
		ITransaction_commit(tra, status);
		/* check error */
		if (IStatus_getState(status)) {
			break;
		}
		tra = NULL;

		printf("Detach from database...\n");
		IAttachment_detach(att, status);
		/* check error */
		if (IStatus_getState(status)) {
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
		IResultSet_release(rs);
		rs = NULL;
	}
	if (meta) {
		IMessageMetadata_release(meta);
		meta = NULL;
	}
	if (stmt) {
		IStatement_release(stmt);
		stmt = NULL;
	}
	if (tra) {
		ITransaction_release(tra);
		tra = NULL;
	}
	if (tpb_builder) {
		IXpbBuilder_dispose(tpb_builder);
		tpb_builder = NULL;
	}
	if (att) {
		IAttachment_release(att);
		att = NULL;
	}
	if (dpb_builder) {
		IXpbBuilder_dispose(dpb_builder);
		dpb_builder = NULL;
	}

	/* check error and print message */
	if (IStatus_getState(status)) {
		char buf[256];
		IUtil_formatStatus(util, buf, sizeof(buf), status);
		printf("Error: %s\n", buf);
		return 1;
	}
	

	return 0;
}