#ifndef VIEW_H
#define VIEW_H

extern const struct xy increment[4];
extern const struct xy left[4];
extern const struct xy right[4];

void DrawView(char (*map)[16], char(*decor)[16], struct xy pos, int direction);

#endif // VIEW_H
