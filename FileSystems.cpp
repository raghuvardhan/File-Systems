#define _CRT_SECURE_NO_WARNINGS
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#define FILESTART 0
#define FILESIZE 64
#define MESSAGESTART 97000064
#define USERSTART 98000000

void messageListFile(FILE *fb, char *fname);

void* allocPage(){
	return calloc(FILESIZE, 1);
}

void* readPage(FILE*fp, int offset){
	void *page = allocPage();
	fseek(fp, offset, SEEK_SET);
	fread(page, FILESIZE, 1, fp);
	return page;
}

void writePage(FILE *fp, int offset, void *memory){
	fseek(fp, offset, SEEK_SET);
	fwrite(memory, FILESIZE, 1, fp);
}

struct filemeta{
	char fname[20];
	char author[20];
	int beginOffset;
	int endOffset;
};

struct filemaster{
	int fileMetaOffset;
	int filesLen;
};

struct filemeta* createFileMeta(FILE *fb, char *author, char *fname, int noOfBlocks){
	struct filemeta* metaFile = (struct filemeta*)malloc(sizeof(struct filemeta));
	strcpy(metaFile->author, author);
	strcpy(metaFile->fname,fname);
	metaFile->beginOffset = ftell(fb) + FILESIZE;
	metaFile->endOffset = metaFile->beginOffset + noOfBlocks * FILESIZE;
	return metaFile;
}

void createFileMaster(FILE *fb){
	struct filemaster* masterFile = (struct filemaster*)malloc(sizeof(struct filemaster));
	masterFile->fileMetaOffset = 0;
	masterFile->filesLen = 0;
	writePage(fb, FILESTART, masterFile);
}

void uploadFile(FILE *fb, char *fname, struct filemeta* metaFile, int nob){
	FILE *write = fopen(fname, "rb+");
	for (int i = 0; i < nob; i++){
		writePage(fb, metaFile->beginOffset + i * FILESIZE, readPage(write, FILESIZE *i));
	}
	fclose(write);
}

int computeFileSize(char *fname){
	FILE *file = fopen(fname, "r");
	fseek(file, 0, SEEK_END);
	return ftell(file);
}

void addFileMeta(FILE *fb, char *author, char *fname){
	int size = computeFileSize(fname);
	int noOfBlocks = size / FILESIZE + 1;
	struct filemaster* masterFile = (struct filemaster*)readPage(fb, FILESTART);
	if (masterFile->filesLen != 0){
		struct filemeta* temp = (struct filemeta*)readPage(fb, masterFile->fileMetaOffset);
		while (temp->endOffset != 0){
			temp = (struct filemeta*)readPage(fb, temp->endOffset);
		}
	}
	struct filemeta* metaFile = createFileMeta(fb, author, fname, noOfBlocks);
	//fprintf(f1, "\nmeta begins %d\nfilebegins %d\nfileends%d\n", ftell(fb), metaFile->beginOffset, metaFile->endOffset);
	writePage(fb, ftell(fb), metaFile);
	if (masterFile->filesLen == 0){
		masterFile->fileMetaOffset = ftell(fb) - FILESIZE;
	}
	masterFile->filesLen++;
	uploadFile(fb, fname, metaFile, noOfBlocks);
	writePage(fb, FILESTART, masterFile);
}

int isFileExists(FILE *fb, char *fname){
	struct filemaster *masterFile = (struct filemaster*)readPage(fb, FILESTART);
	struct filemeta* temp = (struct filemeta*)readPage(fb, masterFile->fileMetaOffset);
	//fprintf(f1, "\n\nisFileExists\nmeta begins\nfilebegins %d\nfileends%d\n", temp->beginOffset, temp->endOffset);

	while (temp->endOffset != 0){
		if (strcmp(temp->fname, fname) == 0){
			return 1;
		}
	//	fprintf(f2, "\n\nisFileExists\nmeta begins\nfilebegins %d\nfileends%d\n", temp->beginOffset, temp->endOffset);
		temp = (struct filemeta*)readPage(fb, temp->endOffset + 64);
	}
	printf("File doesn't exist\n");
	return -1;
}

void download(FILE *fb, char *src){
	if (isFileExists(fb, src) != 1){
		return;
	}
	char dest[20];
	printf("Enter the destination file name:");
	scanf("%s", dest);
	FILE *read = fopen(dest, "wb+");
	struct filemaster *masterFile = (struct filemaster*)readPage(fb, FILESTART);
	struct filemeta* temp = (struct filemeta*)readPage(fb, masterFile->fileMetaOffset);
	while (temp->endOffset != 0){
		if (strcmp(temp->fname, src) == 0){
			int nob = (temp->endOffset - temp->beginOffset) / FILESIZE;
			for (int i = 0; i < nob; i++){
				writePage(read, i * FILESIZE, readPage(fb, temp->beginOffset + i * FILESIZE));
			}
			break;
		}
		temp = (struct filemeta*)readPage(fb, temp->endOffset + 64);
	}
	fclose(read);
}

void listFiles(FILE *fb){
	struct filemaster *masterFile = (struct filemaster*)readPage(fb, FILESTART);
	struct filemeta* temp = (struct filemeta*)readPage(fb, masterFile->fileMetaOffset);
	//fprintf(f2, "\n\nfile reads at file begin %d\n file end %d\n\n", temp->beginOffset, temp->endOffset);
	while (temp->endOffset != 0){
		printf("File Name : %s\n Author: %s\n Messages:\n", temp->fname, temp->author);
		messageListFile(fb, temp->fname);
		temp = (struct filemeta*)readPage(fb, temp->endOffset + 64);
	}
}

struct message{
	char msg[64];
};

struct messagemaster{
	int messageOffSet;
	int messageLen;
};

struct messagemeta{
	char author[20];
	char fname[20];
	int mesg;
};

struct message* createMessage(char *msg){
	struct message* mesg = (struct message*)malloc(sizeof(struct message));
	strcpy(mesg->msg, msg);
	return mesg;
}

struct messagemeta* createMessageMeta(char *author, char *fname){
	struct messagemeta* mesgMeta = (struct messagemeta*)malloc(sizeof(struct messagemeta));
	strcpy(mesgMeta->author, author);
	strcpy(mesgMeta->fname, fname);
	mesgMeta->mesg = 64;
	return mesgMeta;
}

void createMessageMaster(FILE *fb){
	struct messagemaster *masterMessage = (struct messagemaster*)malloc(sizeof(struct messagemaster)); //fb, MESSAGESTART);
	masterMessage->messageLen = 0;
	masterMessage->messageOffSet = 0;
	writePage(fb, MESSAGESTART, masterMessage);
}

void addMessage(FILE *fb, char *author, char *fname){
	if (isFileExists(fb,fname) != 1){
		return;
	}
	char msg[64];
	printf("Enter the message:\n");
	scanf("%s", msg);
	struct messagemaster* masterMessage = (struct messagemaster*)readPage(fb, MESSAGESTART);
	struct messagemeta* metaMessage = createMessageMeta(author, fname);
	writePage(fb, MESSAGESTART + 2 * masterMessage->messageLen * FILESIZE + FILESIZE, metaMessage);
	if (masterMessage->messageLen == 0){
		masterMessage->messageOffSet = ftell(fb) - FILESIZE;
	}
	masterMessage->messageLen++;
	struct message *msgs = createMessage(msg);
	writePage(fb, ftell(fb), msgs);
	writePage(fb, MESSAGESTART, masterMessage);
}

void messageListAuthor(FILE *fb,char *author){
	struct messagemaster *masterMessage = (struct messagemaster*)readPage(fb, MESSAGESTART);
	struct messagemeta *metaMessage = (struct messagemeta*)readPage(fb, masterMessage->messageOffSet);
	for (int i = 0; i < masterMessage->messageLen; i++){
		if (strcmp(metaMessage->author, author) == 0){
			struct message* msg = (struct message*)readPage(fb, ftell(fb));
			printf("Author: %s\n Message: %s\n", author, msg->msg);
		}
		metaMessage = (struct messagemeta*)readPage(fb, MESSAGESTART + (i + 1) * 2 * FILESIZE + FILESIZE);
	}
}

void messageListFile(FILE *fb, char *fname){
	if (isFileExists(fb, fname) != -1){
		return;
	}
	struct messagemaster *masterMessage = (struct messagemaster*)readPage(fb, MESSAGESTART);
	struct messagemeta *metaMessage = (struct messagemeta*)readPage(fb, masterMessage->messageOffSet);
	for (int i = 0; i < masterMessage->messageLen; i++){
		if (strcmp(metaMessage->fname, fname) == 0){
			struct message* msg = (struct message*)readPage(fb, ftell(fb));
			printf("%s\n", msg->msg);
		}
		metaMessage = (struct messagemeta*)readPage(fb, MESSAGESTART + (i + 1) * 2 * FILESIZE + FILESIZE);
	}
}

struct user{
	char username[20];
	char password[20];
};

struct usermaster{
	int userOffset;
	int userLen;
};

void createUserMaster(FILE *fb){
	struct usermaster* masterUser = (struct usermaster*)malloc(sizeof(usermaster));
	masterUser->userLen = 0;
	masterUser->userOffset = 0;
	writePage(fb, USERSTART, masterUser);
}

struct user* createUser(char *uname, char *pwrd){
	struct user* cust = (struct user*)malloc(sizeof(struct user));
	strcpy(cust->username, uname);
	strcpy(cust->password, pwrd);
	return cust;
}

void addUser(FILE *fb, char *uname, char* pwrd){
	struct usermaster* masterUser = (struct usermaster*)readPage(fb, USERSTART);
	struct user *cust = createUser(uname, pwrd);
	writePage(fb, USERSTART + masterUser->userLen * FILESIZE + FILESIZE, cust);
	if (masterUser->userLen == 0){
		masterUser->userOffset = ftell(fb) - FILESIZE;
	}
	masterUser->userLen++;
	writePage(fb, USERSTART, masterUser);
}

int checkUser(FILE *fb, char *uname){
	struct usermaster* masterUser = (struct usermaster*)readPage(fb, USERSTART);
	struct user* cust = (struct user*)readPage(fb, masterUser->userOffset);
	for (int i = 0; i < masterUser->userLen; i++){
		if (strcmp(cust->username, uname) == 0){
			return 1;
		}
		cust = (struct user*)readPage(fb, ftell(fb));
	}
	return 0;
}

int registerUser(FILE *fb, char *userName){
	char pwrd[20];
	printf("Password:\n");
	scanf("%s",pwrd);
	if (checkUser(fb,userName) == 1){
		printf("User Already Exists\n");
		return -1;
	}
	addUser(fb, userName, pwrd);
	printf("Account successfully created\n");
	return 1;
}

int checkAccount(FILE *fb, char *uname, char *pwrd){
	struct usermaster* masterUser = (struct usermaster*)readPage(fb, USERSTART);
	struct user* cust = (struct user*)readPage(fb, masterUser->userOffset);
	for (int i = 0; i < masterUser->userLen; i++){
		if (strcmp(cust->username, uname) == 0 && strcmp(cust->password, pwrd) == 0){
			return 1;
		}
		cust = (struct user*)readPage(fb, ftell(fb));
	}
	return 0;
}

int login(FILE *fb, char *userName){
	char pwrd[20];
	printf("enter the password:");
	scanf("%s", pwrd);
	if (checkAccount(fb, userName, pwrd) == 1){
		printf("logged in\n");
		return 1;
	}
	return -1;
}

void fsInitialise(FILE *fb){
	createFileMaster(fb);
	createMessageMaster(fb);
	createUserMaster(fb);
}

int main(){
	FILE *fb = fopen("read.dat", "wb+");

	fsInitialise(fb);

	int ch = 0, flag = 0;
	char author[20], fname[20];
	printf(" 1.Register\n 2.Login\n");
	scanf("%d", &ch);

	printf("Enter the Username:");
	scanf("%s", author);

	switch (ch){
	case 1:if (registerUser(fb, author) == 1){
		flag = 1;
	}
		break;
	case 2:if (login(fb, author) == 1)
	{
		flag = 1;
	}
		break;
	default:printf("Invalid Input!\n");
		exit(1);
	}

	if (flag == 1){
		do{
			printf("Select your choice:\n 1.Upload a Image\t 2.Download a Image\t 3.List Images\n 4.Comment\t 5.Read Comment\t\n");
			scanf("%d", &ch);

			if (ch != 3 && ch < 6 && ch > 0){
				printf("Enter the ImageName:");
				scanf("%s", fname);
			}

			switch (ch){
			case 1:addFileMeta(fb, author, fname);
				break;
			case 2:download(fb, fname);
				break;
			case 3:listFiles(fb);
				break;
			case 4:addMessage(fb, author, fname);
				break;
			case 5:messageListAuthor(fb, author);
				break;
			default:printf("invalid input!\n");
				break;
			}

		} while (ch <= 5);
	}
	fclose(fb);
	system("pause");
}