import PySimpleGUI as sg
import os
import cv2
import numpy as np
import time
import sys
import os

py_file = os.path.dirname(os.path.realpath(sys.argv[0]))


def wrong(reason):
    layout1 = [[sg.Text(reason,background_color='#FFF0F5',text_color='#000000')]]
    window2 = sg.Window('Error', layout1, size=(225, 40),button_color='#FFC0CB', background_color='#FFF0F5')
    while True:
        event2, values2 = window2.read()
        if event2 == sg.WINDOW_CLOSED or event2 == 'No':
            break
    window2.close()


def code(maxFrame, useHamming, fps):
    dir_path = ''
    dir_path = sg.popup_get_file("Please select the file",background_color='#FFF0F5',text_color='#000000',button_color='#FFC0CB')
    if not dir_path:
        return
    if not os.path.exists(dir_path):
        wrong('File not exits')
        return
    if os.path.exists(py_file + "\\in.mp4"):
        os.remove(py_file + "\\in.mp4")
    print("code:" + py_file + "\\in.mp4")
    cmd = 'encoder ' + dir_path + ' in.mp4 ' + str(maxFrame) + useHamming + ' ' + fps
    sg.popup_quick_message('Encoding...', background_color='white')
    os.chdir(py_file)
    print("code:" + cmd)
    os.system(cmd)
    video_path = py_file + "\\in.mp4"
    confirm_play_video(video_path)


def confirm_play_video(video_path):
    layout1 = [[sg.Text('Do you need to play the video?')],
               [sg.Button('Yes', size=(11, 2)), sg.Button('No', size=(11, 2))]]

    window2 = sg.Window('Finished', layout1, size=(225, 80))
    while True:
        event2, values2 = window2.read()
        if event2 == sg.WINDOW_CLOSED or event2 == 'No':
            break
        elif event2 == 'Yes':
            window2.close()
            play_video(video_path)
            break
    window2.close()


def play_video(file_path):
    if not file_path:
        file_path = sg.popup_get_file("Please select the video",background_color='#FFF0F5',
                                      button_color='#FFC0CB',text_color='#000000')
    if not file_path:
        wrong("Please select the video.")
        return
    if not os.path.exists(file_path):
        wrong('Video not exits!')
        return
    print("play_video:" + file_path)
    video = cv2.VideoCapture(file_path)
    frame_count = int(video.get(cv2.CAP_PROP_FRAME_COUNT))
    fps = int(video.get(cv2.CAP_PROP_FPS))
    window_width = int(video.get(cv2.CAP_PROP_FRAME_WIDTH))
    window_height = int(video.get(cv2.CAP_PROP_FRAME_HEIGHT))
    cv2.namedWindow("Video", cv2.WINDOW_NORMAL)
    cv2.resizeWindow("Video", 1920, 1080)
    cv2.moveWindow("Video", 0, 0)
    if video.get(5) == 0:
        wrong('video is empty')

    bar_width = int(window_width)
    bar_height = 30
    bar_x = int((window_width - bar_width) / 2)
    bar_y = int(window_height - bar_height)
    progress = 0

    def mouse_callback(event3, x, y, flags, param):
        nonlocal progress
        if event3 == cv2.EVENT_LBUTTONDOWN:
            if bar_x <= x <= bar_x + bar_width and bar_y + 30 <= y <= bar_y + bar_height + 30:
                progress = 0
                video.set(cv2.CAP_PROP_POS_FRAMES, int(progress * frame_count))

    cv2.setMouseCallback("Video", mouse_callback)

    while True:
        ret, frame = video.read()

        if ret:
            current_frame = int(video.get(cv2.CAP_PROP_POS_FRAMES))
            progress = current_frame / frame_count

            bar_image = 0 * np.ones((bar_height, bar_width, 3), np.uint8)
            cv2.rectangle(bar_image, (0, 0), (int(bar_width * progress), bar_height), (0, 255, 255), -1)

            combined_image = np.vstack((frame, 0 * np.ones((30, window_width, 3), np.uint8)))
            combined_image[window_height + 30 - bar_height:window_height + 30, bar_x:bar_x + bar_width] = bar_image

            cv2.imshow("Video", combined_image)

        key = cv2.waitKey(1) & 0xFF
        if key == 27 or cv2.getWindowProperty("Video", cv2.WND_PROP_VISIBLE) < 1:
            break
    video.release()
    cv2.destroyAllWindows()


def decode_video(useHamming):
    if not useHamming:
        return
    dir_path_in = sg.popup_get_file('Please select the video',background_color='#FFF0F5',text_color='#000000',button_color='#FFC0CB')
    if not dir_path_in:
        wrong('Please select the video')
        return
    if not os.path.exists(dir_path_in):
        wrong('Video not exits')
        return
    video = cv2.VideoCapture(dir_path_in)
    times = 0
    if video.get(5) != 0:
        times = video.get(7) / video.get(5)
    elif video.get(7) == 0:
        wrong('Video is empty')
        return
    else:
        wrong('Video is empty')
        return
    os.chdir(py_file)
    cmd = 'decoder ' + dir_path_in + ' out.bin ' + useHamming
    sg.popup_quick_message('Decoding...', background_color='white')
    print("decode:" + cmd)
    os.system(cmd)
    fp = open("./res.txt")
    res = fp.read()
    if res == 'Failed to open the video, is the video Incomplete?':
        wrong('Is the video Incomplete?')
        return
    elif res == 'error, there is a skipped frame,there are some images parsed failed.':
        wrong('There is a skipped frame,some images parsed failed.')
    if not os.path.exists('./out.bin'):
        wrong('Missing output')
    outsize = os.path.getsize('./out.bin')

    rate = 0
    res += "VideoTime:" + str(round(times, 2)) + 's\n'
    if times != 0:
        rate = round(outsize / times * 8 / 1024, 2)

    if rate != 0:
        res += "Rate:" + str(rate) + "kb/s"
    layout2 = [[sg.Text(res)],
               [sg.Button('OK')]]
    window3 = sg.Window('Finished', layout2, size=(225, 100))
    while True:
        event3, values3 = window3.read()
        if event3 == sg.WINDOW_CLOSED or event3 == 'OK':
            break
    window3.close()


def code_window():
    fpslist = [10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20]
    in_put = sg.Input(size=(10, 5), key='In', default_text='5000')
    text = sg.Text('  Max Video Time(ms)', size=(18, 1),
                   background_color='#FFC0CB',text_color='#FFFFFF')
    check = sg.Checkbox('Use Hamming Code', size=(15, 1),
                    key='use',background_color='#FFC0CB',text_color='#FFFFFF')
    comb = sg.Combo(fpslist, key='comb', default_value='FPS', size=(15, 1))
    encode2 = sg.Button('Encode', size=(30, 2))
    layout2 = [[text, in_put],
               [check, comb],
               [encode2]]
    window2 = sg.Window('Encode', layout2, size=(265, 120),background_color='#FFF0F5',button_color='#FFC0CB')
    while True:
        event2, values2 = window2.read()
        if event2 == 'Encode':
            if values2['In']:
                maxFrame = values2['In']
            else:
                maxFrame = 1000000
            if values2['use']:
                useHamming = ' 1'
            else:
                useHamming = ' 0'
            if values2['comb'] == 'FPS':
                fps = 15
            else:
                fps = values2['comb']
            code(maxFrame, useHamming, str(fps))
            break
        elif event2 == sg.WINDOW_CLOSED:
            break
    window2.close()


def decode_window():
    check = sg.Checkbox('Use Hamming Code', size=(39, 1), key='use',background_color='#FFC0CB',text_color='#FFFFFF')
    decode2 = sg.Button('Decode', size=(39, 1))
    layout2 = [[check],
               [decode2]]
    window2 = sg.Window('Decode', layout2, size=(265, 80),background_color='#FFF0F5',button_color='#FFC0CB')
    while True:
        event2, values2 = window2.read()
        if event2 == 'Decode':
            if values2['use']:
                useHamming = ' 1'
            else:
                useHamming = ' 0'
            decode_video(useHamming)
            break
        elif event2 == sg.WINDOW_CLOSED:
            break
    window2.close()


if __name__ == '__main__':
    encode = sg.Button('Encode', size=(13, 3))
    decode = sg.Button('Decode', size=(13, 3))
    play = sg.Button('PlayVideo', size=(39, 3))

    layout = [
        [encode, decode],
        [play]
    ]
    window = sg.Window('LightTransmission', layout, size=(265, 145),button_color='#FFC0CB', background_color='#FFF0F5')
    while True:
        event, values = window.read()
        if event == 'Encode':
            code_window()
        elif event == 'Decode':
            decode_window()
        elif event == 'PlayVideo':
            play_video('')
        elif event == sg.WINDOW_CLOSED:
            break
    window.close()