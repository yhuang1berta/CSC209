#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

short combine_mono(char *str1, char *str2);

void reverse_result(char ** return_arr, long int processed_byte, int channel);


short combine_mono(char *str1, char *str2) {
	char str_arr[5];
	str_arr[0] = '\0';
	short x;
	strncat(str_arr, str2, 2);
	strncat(str_arr, str1, 2);
	x = (short)(strtol(str_arr, NULL, 16));
	return x;
}

void reverse_result(char **return_arr, long int processed_byte, int channel) {
	int i;
	if (channel == 1) {
		char temp_buffer[7];
		temp_buffer[6] = '\0';
		temp_buffer[5] = ' ';
		temp_buffer[2] = ' ';
		char hex_str[9];
		hex_str[0] = '\0';
		sprintf(hex_str, "%x", (short)processed_byte);
		if (strlen(hex_str) < 8) {
			char temp[9];
			temp[8] = '\0';
			for (i = 0; i < 8 - strlen(hex_str); i++) {
				temp[i] = '0';
			}
			temp[i] = '\0';
			strncat(temp, hex_str, 8 - strlen(temp));
			for (i = 0; i < 8; i++) {
				hex_str[i] = temp[i];
			}
			hex_str[8] = '\0';
		}
		temp_buffer[1] = hex_str[7];
		temp_buffer[0] = hex_str[6];
		temp_buffer[4] = hex_str[5];
		temp_buffer[3] = hex_str[4];
		*return_arr = temp_buffer;
	} else {
		char hex_str[9];
		int i;
		for (i = 0; i < 9; i++) {
			hex_str[i] = 0;
		}
		sprintf(hex_str, "%lx", (long int)processed_byte);
		// fill unproceeding bits with 0 as place holder
		if (strlen(hex_str) < 8) {
			char temp[9];
			temp[8] = '\0';
			for (i = 0; i < 8 - strlen(hex_str); i++) {
				temp[i] = '0';
			}
			while (i < 8) {
				temp[i] = 0;
				i++;
			}
			strcat(temp, hex_str);
			for (i = 0; i < 8; i++) {
				hex_str[i] = temp[i];
			}
		}
		*return_arr = hex_str;
	}
}

int main(int argc, char **argv) {

	char *func = NULL;
	int time;
	//Handling arguments and error
	if (argc < 3) {
		printf("Error: Invalid command-line arguments. Not enough argument is given! (2 arguments are needed)\n");
		return 1;
	} else if (argc > 3) {
		printf("Error: Invalid command-line arguments. Too many arguments are given! (only 2 are needed)\n");
		return 1;
	} else {
		func = argv[1];
		time = strtol(argv[2], NULL, 10);
		if (!(strcmp(func, "-fin") == 0 || strcmp(func, "-pan") == 0 || strcmp(func, "-fout") == 0)) {
			printf("Error: Invalid command-line arguments. Incorrect first parameter\n");
			return 1;
		}
	}

	char string[100];
	/*this line is read in correctly */
	char *leftover = NULL;
	char *token;
	char *str1 = "", *str2 = "", *str3 = "", *str4 = "";
	// initializing data related variables
	int data_size;
	int rate = 0;
	short channel = 0;
	int fade_sample;
	short proccessed_byte_mono;

	if (time == 0 || (strcmp(func, "-pan") == 0)) {
		while (fgets(string, 100, stdin) != NULL) {
			printf("%s", string);
		}
		return 0;
	}

	if (strcmp(func, "-fin") == 0) {
		int sample_count = 0;
		while (fgets(string, 100, stdin) != NULL) {
			if (string[strlen(string)-1] == '\n') {
				long int line_num = strtol(string, &leftover, 16);
				if (line_num == 0x00000000) {
					printf("%s", string);
				} else if (line_num == 0x00000010) {
					printf("%s", string);
					/*extract number of channel*/
					int i;
					char channel_str[5];
					channel_str[0] = '\0';
					token = strtok(leftover, " ");
					for (i = 0; i < 6; i++) {
						token = strtok(NULL, " ");
					}
					str1 = strtok(NULL, " ");
					str2 = strtok(NULL, " ");
					strcat(channel_str, str2);
					strcat(channel_str, str1);		
					channel = (short)strtol(channel_str, NULL, 16);
					//extract rate
					char rate_str[9];
					rate_str[0] = '\0';
					str1 = strtok(NULL, " ");
					str2 = strtok(NULL, " ");
					str3 = strtok(NULL, " ");
					str4 = strtok(NULL, " ");
					strcat(rate_str, str4);
					strcat(rate_str, str3);
					strcat(rate_str, str2);
					strcat(rate_str, str1);
					rate = strtol(rate_str, NULL, 16);
				} else if (line_num == 0x00000020) {
					printf("00000020");
					// extract data size
					char data_size_str[9];
					data_size_str[0] = '\0';
					int i;
					int index = 10;
					token = strtok(leftover, " ");
					printf("%s ", token);
					for (i = 0; i < 8; i++) {
						token = strtok(NULL, " ");
						printf("%s ", token);
					}
					str1 = strtok(NULL, " ");
					printf("%s ", str1);
					str2 = strtok(NULL, " ");
					printf("%s ", str2);
					str3 = strtok(NULL, " ");
					printf("%s ", str3);
					str4 = strtok(NULL, " ");
					printf("%s ", str4);
					strncat(data_size_str, str4, 2);
					strncat(data_size_str, str3, 2);
					strncat(data_size_str, str2, 2);
					strncat(data_size_str, str1, 2);
					data_size = (int)strtol(data_size_str, NULL, 16);
					//Calculate the number of samples needed to be modified (rounded down)
					fade_sample = (int)((time / 1000.0) * rate);
					if (fade_sample > data_size) {
						fade_sample = data_size;
					}
					if (channel == 2) {
						fade_sample = fade_sample * 2;
					}
					// proccess the remaining 4 bytes in this line
					char *curr_byte;
					if (channel == 1) {
						for (i = sample_count; i < fade_sample * 2; i++) {
							curr_byte = strtok(NULL, " ");
							if (index > 14) {
								printf(" ");
								for (i = 59; i < 75; i++) {
									printf("%c", string[i]);
								}
								printf("\n");
								break;
							} else if ((index%2) == 0) {
								str1 = curr_byte;
								index++;
							} else if ((index%2) == 1) {
								str2 = curr_byte;
								proccessed_byte_mono = ((combine_mono(str1, str2) * (sample_count / (float)fade_sample)));
								if (proccessed_byte_mono < 0) {
									proccessed_byte_mono++;
								}
								sample_count = sample_count + 1;
								char *processed_str;
								reverse_result(&processed_str, proccessed_byte_mono, 1);
								printf("%s", processed_str);
								index++;
							}
						}
					} else {
						for (i = sample_count; i < fade_sample * 2; i++) {
							curr_byte = strtok(NULL, " ");
							if (index > 14) {
								printf(" ");
								for (i = 59; i < 75; i++) {
									printf("%c", string[i]);
								}
								printf("\n");
								break;
							} else if ((index%4) == 0) {
								str1 = curr_byte;
								index++;
							} else if ((index%4) == 1) {
								str2 = curr_byte;
								index++;
							} else if ((index%4) == 2) {
								str3 = curr_byte;
								index++;
							} else if ((index%4) == 3) {
								str4 = curr_byte;
								proccessed_byte_mono = (int)((combine_mono(str1, str2) * (sample_count / (float)fade_sample)));
								if (proccessed_byte_mono < 0) {
									proccessed_byte_mono++;
								}
								char *processed_str;
								reverse_result(&processed_str, proccessed_byte_mono, 1);
								printf("%s", processed_str);
								proccessed_byte_mono = (int)((combine_mono(str3, str4) * (sample_count / (float)fade_sample)));
								if (proccessed_byte_mono < 0) {
									proccessed_byte_mono++;
								}
								sample_count = sample_count + 2;
								reverse_result(&processed_str, proccessed_byte_mono, 1);
								printf("%s", processed_str);
								index++;
							}
						}
					}
				} else {
					// line beyond 0x00000020
					char *curr_byte = NULL;
					char *line_num_str;
					reverse_result(&line_num_str, line_num, 2);
					int i;
					for (i = 0; i < 8; i++) {
						printf("%c",line_num_str[i]);
					}
					printf(": ");
					int index = 0;
					if (channel == 1) {
						curr_byte = strtok(leftover, " ");
						// printf("strlen of curr_byte: %ld\n", strlen(curr_byte));
						if (sample_count < fade_sample) {
							while (sample_count < fade_sample) {
								if (index > 15) {
									printf(" ");
									for (i = 59; i < 75; i++) {
										if (string[i] == '\n') {
											break;
										}
										printf("%c", string[i]);
									}
									printf("\n");
									break;
								} else if ((index%2) == 0) {
									str1 = strtok(NULL, " ");
									if (str1 == NULL) {
										str1 = "  ";
									}
									index++;
								} else if ((index%2) == 1){
									str2 = strtok(NULL, " ");
									if (str2 == NULL) {
										str2 = "  ";
									}
									if (str1[0] == ' ' || str2[0] == ' ') {
										str1 = "  ";
										str2 = "  ";
										printf("%s %s ", str2, str1);
										index++;
									} else {
										proccessed_byte_mono = (int)((combine_mono(str1, str2) * (sample_count / (float)fade_sample)));
										if (proccessed_byte_mono < 0) {
											proccessed_byte_mono++;
										}
										sample_count = sample_count + 1;
										index++;
										char *processed_str;
										// reverse_result returns strings in big endian
										reverse_result(&processed_str, proccessed_byte_mono, 1);
										// swap for little endian
										printf("%s", processed_str);
										// if sample is cut off mid way due to fade time
										if (sample_count >= fade_sample) {
											while (curr_byte[strlen(curr_byte)-1] != '\n') {
												curr_byte = strtok(NULL, " ");
												if (index > 15) {
													printf(" ");
													for (i = 59; i < 75; i++) {
														printf("%c", string[i]);
													}
													printf("\n");
													break;
												} else {
													if (curr_byte == NULL) {
														printf("   ");
													} else {
														printf("%s ", curr_byte);
													}
													index++;
												}
											}
										}
									}
								}
							}
						
						} else {
							curr_byte = strtok(NULL, "");
							printf("%s", curr_byte);
						}
					} else if (channel == 2) {
						char *processed_str;
						curr_byte = strtok(leftover, " ");
						if (sample_count < fade_sample) {
							while (sample_count < fade_sample) {
								if (index > 15) {
									printf(" ");
									for (i = 59; i < 74; i++) {
										printf("%c", string[i]);
									}
									printf("\n");
									break;
								} else if ((index%4) == 0) {
									str1 = strtok(NULL, " ");
									index++;
								} else if ((index%4) == 1) {
									str2 = strtok(NULL, " ");
									index++;
								}  else if ((index%4) == 2) {
									str3 = strtok(NULL, " ");
									index++;
								}  else if ((index%4) == 3){ 
									str4 = strtok(NULL, " ");
									index++;
									proccessed_byte_mono = (int)((combine_mono(str1, str2) * (sample_count / (float)fade_sample)));
									if (proccessed_byte_mono < 0) {
										proccessed_byte_mono++;
									}
									reverse_result(&processed_str, proccessed_byte_mono, 1);
									printf("%s", processed_str);
									proccessed_byte_mono = (int)((combine_mono(str3, str4) * (sample_count / (float)fade_sample)));
									if (proccessed_byte_mono < 0) {
										proccessed_byte_mono++;
									}
									reverse_result(&processed_str, proccessed_byte_mono, 1);
									printf("%s", processed_str);
									sample_count = sample_count + 4;
					
									// if sample is cut off mid way due to fade time
									if (sample_count >= fade_sample) {
										while (curr_byte[strlen(curr_byte)-1] != '\n') {
											curr_byte = strtok(NULL, " ");
											if (index > 15) {
												printf(" ");
												for (i = 59; i < 75; i++) {
													printf("%c", string[i]);
												}
												printf("\n");
												break;
											} else {
												printf("%s ", curr_byte);
												index++;
											}	
										}
									}
								}
							}
						
						} else {
							curr_byte = strtok(NULL, "");
							printf("%s", curr_byte);
						}
					}
				}
			} else {
				printf("%s\n", "string was truncated");
			}
		}
	} else if (strcmp(func, "-fout") == 0) {
		int sample_count = 0;
		// int index;
		while (fgets(string, 100, stdin) != NULL) {
			if (string[strlen(string)-1] == '\n') {
				long int line_num = strtol(string, &leftover, 16);
				if (line_num == 0x00000000) {
					printf("%s", string);
				} else if (line_num == 0x00000010) {
					printf("%s", string);
					/*extract number of channel*/
					int i;
					char channel_str[5];
					channel_str[0] = '\0';
					token = strtok(leftover, " ");
					for (i = 0; i < 6; i++) {
						token = strtok(NULL, " ");
					}
					str1 = strtok(NULL, " ");
					str2 = strtok(NULL, " ");
					strcat(channel_str, str2);
					strcat(channel_str, str1);		
					channel = (short)strtol(channel_str, NULL, 16);
					//extract rate
					char rate_str[9];
					rate_str[0] = '\0';
					str1 = strtok(NULL, " ");
					str2 = strtok(NULL, " ");
					str3 = strtok(NULL, " ");
					str4 = strtok(NULL, " ");
					strcat(rate_str, str4);
					strcat(rate_str, str3);
					strcat(rate_str, str2);
					strcat(rate_str, str1);
					rate = strtol(rate_str, NULL, 16);
				} else if (line_num == 0x00000020) {
					printf("00000020");
					// extract data size
					char data_size_str[9];
					data_size_str[0] = '\0';
					int i;
					token = strtok(leftover, " ");
					printf("%s ", token);
					for (i = 0; i < 8; i++) {
						token = strtok(NULL, " ");
						printf("%s ", token);
					}
					str1 = strtok(NULL, " ");
					printf("%s ", str1);
					str2 = strtok(NULL, " ");
					printf("%s ", str2);
					str3 = strtok(NULL, " ");
					printf("%s ", str3);
					str4 = strtok(NULL, " ");
					printf("%s ", str4);
					strncat(data_size_str, str4, 2);
					strncat(data_size_str, str3, 2);
					strncat(data_size_str, str2, 2);
					strncat(data_size_str, str1, 2);
					data_size = (int)strtol(data_size_str, NULL, 16);
					//Calculate the number of samples needed to be modified (rounded down)
					fade_sample = (int)((time / 1000.0) * rate);
					if (fade_sample > data_size) {
						fade_sample = data_size;
					}
					if (channel == 2) {
						fade_sample = fade_sample * 2;
					}
					if (data_size - fade_sample > 4) {
						int i;
						for (i = 46; i < 75; i++) {
							printf("%c", string[i]);
						}
						printf("\n");
						sample_count = sample_count + 2;
					} else {
						int index = 10;
						if (channel == 1) {
							while (1) {
								if (index > 13) {
									int i;
									printf(" ");
									for (i = 59; i < 75; i++) {
										printf("%c", string[i]);
									}
									printf("\n");
									break;
								
								} else if ((index%2) == 0) {
									str1 = strtok(NULL, " ");
									index++;
								} else if ((index%2) == 1) {
									str2 = strtok(NULL, " ");
									proccessed_byte_mono = (combine_mono(str1, str2) * ((fade_sample-sample_count)/(float)fade_sample));
									if (proccessed_byte_mono < 0) {
										proccessed_byte_mono++;
									}
									char *processed_str;
									reverse_result(&processed_str, proccessed_byte_mono, 1);
									printf("%s", processed_str);
									sample_count++;
									index++;
								}
							}
						} else {
							str1 = strtok(NULL, " ");
							str2 = strtok(NULL, " ");
							proccessed_byte_mono = (combine_mono(str1, str2) * ((fade_sample-sample_count)/(float)fade_sample));
							char *processed_str;
							reverse_result(&processed_str, proccessed_byte_mono, 1);
							printf("%s", processed_str);
							str1 = strtok(NULL, " ");
							str2 = strtok(NULL, " ");
							proccessed_byte_mono = (combine_mono(str1, str2) * ((fade_sample-sample_count)/(float)fade_sample));
							if (proccessed_byte_mono < 0) {
								proccessed_byte_mono++;
							}
							reverse_result(&processed_str, proccessed_byte_mono, 1);
							printf("%s", processed_str);
							sample_count = sample_count + 4;
							printf(" ");
							int i;
							for (i = 59; i < 75; i++) {
								printf("%c", string[i]);
							}
							printf("\n");
							break;
						}
					}
				} else {
					// line beyond 0x00000020
					char *curr_byte = strtok(leftover, " ");
					char *line_num_str;
					reverse_result(&line_num_str, line_num, 2);
					int i;
					for (i = 0; i < 8; i++) {
						printf("%c",line_num_str[i]);
					}
					printf(": ");
					if (channel == 1) {
						int index = 0;
						if (data_size - sample_count > fade_sample) {
							while ((data_size - sample_count) > fade_sample) {
								if (index > 15) {
									int i;
									printf(" ");
									for (i = 59; i < 75; i++) {
										printf("%c", string[i]);
									}
									printf("\n");
									break;
								} else {
									curr_byte = strtok(NULL, " ");
									printf("%s ", curr_byte);
									index++;
									sample_count++;
								}
							}
							if ((data_size - sample_count) <= fade_sample) {
								char *processed_str;
								while (1) {
									if (index > 15) {
										int i;
										printf(" ");
										for (i = 59; i < 75; i++) {
											if (string[i] == '\n') {
												break;
											}
											printf("%c", string[i]);
										}
										printf("\n");
										break;
									} else if ((index%2) == 0) {
										str1 = strtok(NULL, " ");
										index++;
									} else if ((index%2) == 1) {
										str2 = strtok(NULL, " ");
										proccessed_byte_mono = (int)((combine_mono(str1, str2) * ((data_size - sample_count) / (float)fade_sample)));
										if (proccessed_byte_mono < 0) {
										proccessed_byte_mono++;
										}
										reverse_result(&processed_str, proccessed_byte_mono, 1);
										printf("%s", processed_str);
										index++;
										sample_count += 1;
									}
								}
							}
						} else {
							char *processed_str;
							int i;
							while (1) {
								if (index > 15) {
									short flag = 0;
									printf(" ");
									for (i = 59; i < 75; i++) {
										printf("%c", string[i]);
										if (string[i] == '\n') {
											flag = 1;
											break;
										}
									}
									if (flag == 0) {
										printf("\n");
									}
									break;
								} else if ((index%2) == 0) {
									str1 = strtok(NULL, " ");
									if (str1 == NULL) {
										str1 = "  ";
									}
									index++;
								} else if ((index%2) == 1) {
									str2 = strtok(NULL, " ");
									if (str2 == NULL) {
										str2 = "  ";
									}
									if (str1[0] == ' ' || str2[0] == ' ') {
										printf("%s %s ", str2, str1);
									} else {
										proccessed_byte_mono = (int)((combine_mono(str1, str2) * ((data_size - sample_count) / (float)fade_sample)));
										if (proccessed_byte_mono < 0) {
											proccessed_byte_mono++;
										}
										reverse_result(&processed_str, proccessed_byte_mono, 1);
										printf("%s", processed_str);
									}
									index++;
									sample_count += 4;
								}
							}
						}
					} else if (channel == 2) {
						int index = 0;
						if (data_size - sample_count > fade_sample) {
							while ((data_size - sample_count) > fade_sample) {
								if (index > 15) {
									int i;
									printf(" ");
									for (i = 59; i < 75; i++) {
										printf("%c", string[i]);
									}
									printf("\n");
									break;
								} else {
									curr_byte = strtok(NULL, " ");
									printf("%s ", curr_byte);
									index++;
									sample_count++;
								}
							}
							if ((data_size - sample_count) <= fade_sample) {
								char *processed_str;
								while (1) {
									if (index > 15) {
										int i;
										printf(" ");
										for (i = 59; i < 75; i++) {
											printf("%c", string[i]);
										}
										printf("\n");
										break;
									} else if ((index%4) == 0) {
										str1 = strtok(NULL, " ");
										index++;
									} else if ((index%4) == 1) {
										str2 = strtok(NULL, " ");
										index++;
										proccessed_byte_mono = (int)((combine_mono(str1, str2) * ((data_size - sample_count) / (float)fade_sample)));
										if (proccessed_byte_mono < 0) {
										proccessed_byte_mono++;
										}
										reverse_result(&processed_str, proccessed_byte_mono, 1);
										printf("%s", processed_str);
									} else if ((index%4) == 2) {
										str3 = strtok(NULL, " ");
										index++;
									} else if ((index%4) == 3) {
										str4 = strtok(NULL, " ");
										proccessed_byte_mono = (int)((combine_mono(str3, str4) * ((data_size - sample_count) / (float)fade_sample)));
										if (proccessed_byte_mono < 0) {
										proccessed_byte_mono++;
										}
										reverse_result(&processed_str, proccessed_byte_mono, 1);
										printf("%s", processed_str);
										index++;
										sample_count += 4;
									}
								}
							}
						} else {
							int index = 0;
							char *processed_str;
							int i;
							while (1) {
								if (index > 15) {
									short flag = 0;
									printf(" ");
									for (i = 59; i < 75; i++) {
										printf("%c", string[i]);
										if (string[i] == '\n') {
											flag = 1;
											break;
										}
									}
									if (flag == 0) {
										printf("\n");
									}
									break;
								} else if ((index%4) == 0) {
									str1 = strtok(NULL, " ");
									if (str1 == NULL) {
										str1 = "  ";
									}
									index++;
								} else if ((index%4) == 1) {
									str2 = strtok(NULL, " ");
									index++;
									if (str2 == NULL) {
										str1 = "  ";
										str2 = "  ";
									}
									if (str1[0] == ' ' || str2[0] == ' ') {
										printf("%s %s ", str2, str1);
									} else {
										proccessed_byte_mono = (int)((combine_mono(str1, str2) * ((data_size - sample_count) / (float)fade_sample)));
										if (proccessed_byte_mono < 0) {
											proccessed_byte_mono++;
										}
										reverse_result(&processed_str, proccessed_byte_mono, 1);
										printf("%s", processed_str);
									}
								} else if ((index%4) == 2) {
									str3 = strtok(NULL, " ");
									if (str3 == NULL) {
										str3 = "  ";
									}
									index++;
								} else if ((index%4) == 3) {
									str4 = strtok(NULL, " ");
									if (str4 == NULL) {
										str4 = "  ";
									}
									if (str3[0] == ' ' || str4[0] == ' ') {
										printf("%s %s ", str4, str3);
									} else {
										proccessed_byte_mono = (int)((combine_mono(str3, str4) * ((data_size - sample_count) / (float)fade_sample)));
										if (proccessed_byte_mono < 0) {
											proccessed_byte_mono++;
										}
										reverse_result(&processed_str, proccessed_byte_mono, 1);
										printf("%s", processed_str);
									}
									index++;
									sample_count += 4;
								}
							}
						}
					}
				}
			}
		}
	}
	return 0;
}
