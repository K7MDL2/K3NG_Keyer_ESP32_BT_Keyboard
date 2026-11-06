#ifndef KEYER_KEYBOARD_CODES_H
#define KEYER_KEYBOARD_CODES_H

// Internal BT/touch command bytes. Preserve values for existing layouts.
#define BT_KEY_TAB         9
#define BT_KEY_ENTER       13
#define BT_KEY_BACKSPACE   127
#define BT_KEY_ESC         27
#define BT_KEY_INSERT      128
#define BT_KEY_DELETE      127
#define BT_KEY_HOME        129
#define BT_KEY_END         156
#define BT_KEY_PAGEUP      25
#define BT_KEY_PAGEDOWN    26
#define BT_KEY_UPARROW     11
#define BT_KEY_LEFTARROW   8
#define BT_KEY_DOWNARROW   10
#define BT_KEY_RIGHTARROW  21
#define BT_KEY_F1          130
#define BT_KEY_F2          131
#define BT_KEY_F3          132
#define BT_KEY_F4          133
#define BT_KEY_F5          134
#define BT_KEY_F6          135
#define BT_KEY_F7          136
#define BT_KEY_F8          137
#define BT_KEY_F9          138
#define BT_KEY_F10         139
#define BT_KEY_F11         140
#define BT_KEY_F12         141
#define BT_KEY_SCROLL      142

#define BT_KEY_F1_SHIFT          143
#define BT_KEY_F2_SHIFT          144
#define BT_KEY_F3_SHIFT          145
#define BT_KEY_F4_SHIFT          146
#define BT_KEY_F5_SHIFT          147
#define BT_KEY_F6_SHIFT          148
#define BT_KEY_F7_SHIFT          149
#define BT_KEY_F8_SHIFT          150
#define BT_KEY_F9_SHIFT          151
#define BT_KEY_F10_SHIFT         152
#define BT_KEY_F11_SHIFT         153
#define BT_KEY_F12_SHIFT         154
#define BT_KEY_TAB_SHIFT         9
#define BT_KEY_ENTER_SHIFT       13
#define BT_KEY_BACKSPACE_SHIFT   157

#define BT_KEY_LEFT_ALT	158
#define BT_KEY_SCROLL_SHIFT 160

#if defined(OPTION_BT_KEYBOARD_US)
#define BT_KEY_A_CTRL 162
#define BT_KEY_B_CTRL 163
#define BT_KEY_C_CTRL 164
#define BT_KEY_D_CTRL 165
#define BT_KEY_E_CTRL 166
#define BT_KEY_F_CTRL 167
#define BT_KEY_G_CTRL 168
#define BT_KEY_H_CTRL 169
#define BT_KEY_I_CTRL 170
#define BT_KEY_J_CTRL 171
#define BT_KEY_K_CTRL 172
#define BT_KEY_L_CTRL 173
#define BT_KEY_M_CTRL 174
#define BT_KEY_N_CTRL 175
#define BT_KEY_O_CTRL 176
#define BT_KEY_P_CTRL 177
#define BT_KEY_Q_CTRL 178
#define BT_KEY_R_CTRL 179
#define BT_KEY_S_CTRL 180
#define BT_KEY_T_CTRL 181
#define BT_KEY_U_CTRL 182
#define BT_KEY_V_CTRL 183
#define BT_KEY_W_CTRL 184
#define BT_KEY_X_CTRL 185
#define BT_KEY_Y_CTRL 186
#define BT_KEY_Z_CTRL 187
#define BT_KEY_F1_CTRL 188
#define BT_KEY_F2_CTRL 189
#define BT_KEY_F3_CTRL 190
#define BT_KEY_F4_CTRL 191
#define BT_KEY_F5_CTRL 192
#define BT_KEY_F6_CTRL 193
#define BT_KEY_F7_CTRL 194
#define BT_KEY_F8_CTRL 195
#define BT_KEY_F9_CTRL 196
#define BT_KEY_F10_CTRL 197
#define BT_KEY_F11_CTRL 198
#define BT_KEY_F12_CTRL 199
#define BT_KEY_F1_ALT 200
#define BT_KEY_F2_ALT 201
#define BT_KEY_F3_ALT 202
#define BT_KEY_F4_ALT 203
#define BT_KEY_F5_ALT 204
#define BT_KEY_F6_ALT 205
#define BT_KEY_F7_ALT 206
#define BT_KEY_F8_ALT 207
#define BT_KEY_F9_ALT 208
#define BT_KEY_F10_ALT 209
#define BT_KEY_F11_ALT 210
#define BT_KEY_F12_ALT 211
#endif //OPTION_BT_KEYBOARD_US


#if defined(OPTION_BT_KEYBOARD_GERMAN)

#define BT_KEY_A_CTRL 162
#define BT_KEY_B_CTRL 163
#define BT_KEY_C_CTRL 200 //164
#define BT_KEY_D_CTRL 165
#define BT_KEY_E_CTRL 166
#define BT_KEY_F_CTRL 201 //167
#define BT_KEY_G_CTRL 168
#define BT_KEY_H_CTRL 169
#define BT_KEY_I_CTRL 170
#define BT_KEY_J_CTRL 171
#define BT_KEY_K_CTRL 172
#define BT_KEY_L_CTRL 173
#define BT_KEY_M_CTRL 174
#define BT_KEY_N_CTRL 175
#define BT_KEY_O_CTRL 202 //176
#define BT_KEY_P_CTRL 177
#define BT_KEY_Q_CTRL 203 //178
#define BT_KEY_R_CTRL 204 //179
#define BT_KEY_S_CTRL 180
#define BT_KEY_T_CTRL 205 //181
#define BT_KEY_U_CTRL 182
#define BT_KEY_V_CTRL 183
#define BT_KEY_W_CTRL 184
#define BT_KEY_X_CTRL 185
#define BT_KEY_Y_CTRL 186
#define BT_KEY_Z_CTRL 187
#define BT_KEY_F1_CTRL 188
#define BT_KEY_F2_CTRL 189
#define BT_KEY_F3_CTRL 190
#define BT_KEY_F4_CTRL 191
#define BT_KEY_F5_CTRL 192
#define BT_KEY_F6_CTRL 193
#define BT_KEY_F7_CTRL 194
#define BT_KEY_F8_CTRL 195
#define BT_KEY_F9_CTRL 206 //196
#define BT_KEY_F10_CTRL 197
#define BT_KEY_F11_CTRL 198
#define BT_KEY_F12_CTRL 199
#define BT_KEY_F1_ALT 229
#define BT_KEY_F2_ALT 230
#define BT_KEY_F3_ALT 231
#define BT_KEY_F4_ALT 232
#define BT_KEY_F5_ALT 233
#define BT_KEY_F6_ALT 234
#define BT_KEY_F7_ALT 235
#define BT_KEY_F8_ALT 236
#define BT_KEY_F9_ALT 237
#define BT_KEY_F10_ALT 238
#define BT_KEY_F11_ALT 239
#define BT_KEY_F12_ALT 240

#define BT_KEY_CURRENCY_SIGN   164 // ¤
#define BT_KEY_SECTION_SIGN    167 // §
#define BT_KEY_DEGREE_SIGN     176 // °
#define BT_KEY_SUPERSCRIPT_TWO   178 // ²
#define BT_KEY_SUPERSCRIPT_THREE   179 // ³
#define BT_KEY_MICRO_SIGN      181 // µ
#define BT_KEY_A_DIAERESIS     196 // Ä
#define BT_KEY_O_DIAERESIS     214 // Ö
#define BT_KEY_U_DIAERESIS     220 // Ü
#define BT_KEY_SHARP_S     223 // ß
#define BT_KEY_a_DIAERESIS     228 // ä
#define BT_KEY_o_DIAERESIS     246 // ö
#define BT_KEY_u_DIAERESIS     252 // ü
#endif //OPTION_BT_KEYBOARD_GERMAN

#if defined(OPTION_BT_KEYBOARD_FRENCH)
#define BT_KEY_A_CTRL 162
#define BT_KEY_B_CTRL 200 //163
#define BT_KEY_C_CTRL 164
#define BT_KEY_D_CTRL 165
#define BT_KEY_E_CTRL 166
#define BT_KEY_F_CTRL 201 //167
#define BT_KEY_G_CTRL 202 //168
#define BT_KEY_H_CTRL 169
#define BT_KEY_I_CTRL 170
#define BT_KEY_J_CTRL 171
#define BT_KEY_K_CTRL 172
#define BT_KEY_L_CTRL 173
#define BT_KEY_M_CTRL 174
#define BT_KEY_N_CTRL 175
#define BT_KEY_O_CTRL 203 //176
#define BT_KEY_P_CTRL 177
#define BT_KEY_Q_CTRL 204 //178
#define BT_KEY_R_CTRL 179
#define BT_KEY_S_CTRL 180
#define BT_KEY_T_CTRL 205 //181
#define BT_KEY_U_CTRL 182
#define BT_KEY_V_CTRL 183
#define BT_KEY_W_CTRL 184
#define BT_KEY_X_CTRL 185
#define BT_KEY_Y_CTRL 186
#define BT_KEY_Z_CTRL 187
#define BT_KEY_F1_CTRL 188
#define BT_KEY_F2_CTRL 189
#define BT_KEY_F3_CTRL 190
#define BT_KEY_F4_CTRL 191
#define BT_KEY_F5_CTRL 192
#define BT_KEY_F6_CTRL 193
#define BT_KEY_F7_CTRL 194
#define BT_KEY_F8_CTRL 195
#define BT_KEY_F9_CTRL 196
#define BT_KEY_F10_CTRL 197
#define BT_KEY_F11_CTRL 198
#define BT_KEY_F12_CTRL 199
#define BT_KEY_F1_ALT 233
#define BT_KEY_F2_ALT 234
#define BT_KEY_F3_ALT 235
#define BT_KEY_F4_ALT 236
#define BT_KEY_F5_ALT 237
#define BT_KEY_F6_ALT 238
#define BT_KEY_F7_ALT 239
#define BT_KEY_F8_ALT 240
#define BT_KEY_F9_ALT 241
#define BT_KEY_F10_ALT 242
#define BT_KEY_F11_ALT 243
#define BT_KEY_F12_ALT 244

#define BT_KEY_POUND_SIGN      163 // £
#define BT_KEY_SECTION_SIGN    167 // §
#define BT_KEY_DIAERESIS     168 // ¨
#define BT_KEY_DEGREE_SIGN     176 // °
#define BT_KEY_SUPERSCRIPT_TWO   178 // ²
#define BT_KEY_MICRO_SIGN      181 // µ
#define BT_KEY_a_GRAVE     224 // à
#define BT_KEY_c_CEDILLA     231 // ç
#define BT_KEY_e_GRAVE     232 // è
#define BT_KEY_e_ACUTE     233 // é
#define BT_KEY_u_GRAVE     249 // ù
#endif //OPTION_BT_KEYBOARD_FRENCH

#endif
