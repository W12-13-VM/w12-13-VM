#include "filesys/file.h"
#include <debug.h>
#include "filesys/inode.h"
#include "threads/malloc.h"

/* Opens a file for the given INODE, of which it takes ownership,
 * and returns the new file.  Returns a null pointer if an
 * allocation fails or if INODE is null. */

void increase_dup_count(struct file *file)
{
	file->dup_count++;
}

void decrease_dup_count(struct file *file)
{
	file->dup_count--;
}

int check_dup_count(struct file *file)
{
	return file->dup_count;
}


void increase_mapping_count(struct file *file)
{
	file->mapping_cnt++;
}

void decrease_mapping_count(struct file *file)
{
	file->mapping_cnt--;
}

int check_mapping_count(struct file *file)
{
	return file->mapping_cnt;
}

/*
주어진 inode를 가진 file 구조체를 만들어 반환 .
*/
struct file *
file_open(struct inode *inode)
{
	struct file *file = calloc(1, sizeof *file);
	if (inode != NULL && file != NULL)
	{
		file->inode = inode;
		file->pos = 0;
		/* extra 2 */
		file->dup_count = 1;
		file->deny_write = false;
		return file;
	}
	else
	{
		inode_close(inode);
		free(file);
		return NULL;
	}
}

/* FILE과 동일한 inode를 가지는 새로운 파일을 열어 반환합니다.
 * 실패하면 null 포인터를 반환합니다. */
struct file *
file_reopen(struct file *file)
{
	return file_open(inode_reopen(file->inode));
}

/* 파일 객체를 속성 포함하여 복제하고, FILE과 동일한 inode에 대한 새 파일을 반환합니다.
 * 실패한 경우 null 포인터를 반환합니다. */
struct file *
file_duplicate(struct file *file)
{
	struct file *nfile = file_open(inode_reopen(file->inode));
	if (nfile)
	{
		nfile->pos = file->pos;
		if (file->deny_write)
			file_deny_write(nfile);
	}
	return nfile;
}

/* Closes FILE. */
void file_close(struct file *file)
{
	if (file != NULL)
	{
		file_allow_write(file);
		inode_close(file->inode);
		free(file);
	}
}

/* Returns the inode encapsulated by FILE. */
struct inode *
file_get_inode(struct file *file)
{
	return file->inode;
}

/* FILE에서 BUFFER로 SIZE 바이트를 읽습니다.
 * 파일의 현재 위치에서 시작합니다.
 * 실제로 읽은 바이트 수를 반환하며,
 * 파일 끝에 도달하면 SIZE보다 적을 수 있습니다.
 * FILE의 위치를 읽은 바이트 수만큼 이동시킵니다. */
off_t file_read(struct file *file, void *buffer, off_t size)
{
	off_t bytes_read = inode_read_at(file->inode, buffer, size, file->pos);
	file->pos += bytes_read;
	return bytes_read;
}

/* FILE에서 BUFFER로 SIZE 바이트를 읽습니다.
 * 파일의 FILE_OFS 오프셋에서 시작합니다.
 * 실제로 읽은 바이트 수를 반환하며,
 * 파일 끝에 도달하면 SIZE보다 적을 수 있습니다.
 * FILE의 현재 위치는 영향을 받지 않습니다. */
off_t file_read_at(struct file *file, void *buffer, off_t size, off_t file_ofs)
{
	return inode_read_at(file->inode, buffer, size, file_ofs);
}

/* FILE의 현재 위치에서 시작하여,
 * BUFFER에서 FILE로 SIZE 바이트를 씁니다.
 * 실제로 기록된 바이트 수를 반환하며,
 * 파일 끝에 도달하면 SIZE보다 적을 수 있습니다.
 * (일반적으로 이 경우 파일을 확장해야 하지만,
 * 파일 확장은 아직 구현되지 않았습니다.)
 * 기록된 바이트 수만큼 FILE의 위치를 이동시킵니다. */
off_t file_write(struct file *file, const void *buffer, off_t size)
{
	off_t bytes_written = inode_write_at(file->inode, buffer, size, file->pos);
	file->pos += bytes_written;
	return bytes_written;
}

/* BUFFER에서 FILE로 SIZE 바이트를 씁니다.
 * 파일의 FILE_OFS 오프셋에서 시작합니다.
 * 실제로 기록된 바이트 수를 반환하며,
 * 파일 끝에 도달하면 SIZE보다 적을 수 있습니다.
 * (일반적으로 이 경우 파일을 확장해야 하지만,
 * 파일 확장은 아직 구현되지 않았습니다.)
 * FILE의 현재 위치는 영향을 받지 않습니다. */
off_t file_write_at(struct file *file, const void *buffer, off_t size,
					off_t file_ofs)
{
	return inode_write_at(file->inode, buffer, size, file_ofs);
}

/* Prevents write operations on FILE's underlying inode
 * until file_allow_write() is called or FILE is closed. */
void file_deny_write(struct file *file)
{
	ASSERT(file != NULL);
	if (!file->deny_write)
	{
		file->deny_write = true;
		inode_deny_write(file->inode);
	}
}

/* Re-enables write operations on FILE's underlying inode.
 * (Writes might still be denied by some other file that has the
 * same inode open.) */
void file_allow_write(struct file *file)
{
	ASSERT(file != NULL);
	if (file->deny_write)
	{
		file->deny_write = false;
		inode_allow_write(file->inode);
	}
}

/* Returns the size of FILE in bytes. */
off_t file_length(struct file *file)
{
	ASSERT(file != NULL);
	return inode_length(file->inode);
}

/* FILE의 현재 위치를 파일의 시작점으로부터 NEW_POS 바이트로 설정합니다. */
void file_seek(struct file *file, off_t new_pos)
{
	ASSERT(file != NULL);
	ASSERT(new_pos >= 0);
	file->pos = new_pos;
}

/* FILE의 현재 위치를 파일의 시작점으로부터 바이트 단위로 반환합니다. */
off_t file_tell(struct file *file)
{
	ASSERT(file != NULL);
	return file->pos;
}
