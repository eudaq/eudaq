#!/usr/bin/env python3

import os
import json
import time
import requests
import argparse
import smtplib
from smtplib import SMTPRecipientsRefused, SMTPHeloError, SMTPSenderRefused, SMTPDataError, SMTPNotSupportedError

class MessagePigeon:
    def __init__(self):
        self._message_service_dict = {}
        self._mail_server_object = None
        self._mail_sender_addr = None
    
    def _send_to_mattermost(self, message, id):
        webhook = self._message_service_dict[id]['webhook']
        channel = self._message_service_dict[id]['channel']

        reply = requests.post(
            webhook,
            headers={'Content-Type': 'application/json'},
            data=json.dumps({'text':message,'channel':channel})
        )
        if reply.text == 'ok':
            return True
        else:
            print(f'Warning: Could not send mattermost message to {id}. \nDumping return message:')
            print(reply.json())
            return False

    def _send_mail(self, recv_address, message_subject, message):
        try:
            msg = message_subject + message
            self._mail_server_object.sendmail(self._mail_sender_addr, recv_address, msg)
            return True
        except:
            print(f'Warning: Could not send mail to {recv_address}.')
            return False

    def _send_to_email(self, recv_address, message_subject, message):
        header = 'To:' + recv_address + '\n' + 'From: ' + self._mail_sender_addr + '\n' + 'Subject: ' + message_subject + '\n\n'
        msg = message
        return self._send_mail(recv_address=recv_address, message_subject=header, message=msg)

    def _send_to_phone(self, recv_phone_number, message_subject, message):
        recv_phone_number = recv_phone_number + '@mail2sms.cern.ch'
        header = 'To:' + recv_phone_number + '\n' + 'From:' + self._mail_sender_addr + '\n' + 'Subject:' + message_subject + '\n\n'
        msg = header + message
        return self._send_mail(recv_address=recv_phone_number, message_subject=header, message=msg)

    def start_mail_service(self, server='cernmx.cern.ch', port=25, sender_addr='Pigeon-noreply@cern.ch'):
        self._mail_service_dict = {'server_object' : None, 'sender_addr' : None}
        self._mail_server_object = smtplib.SMTP('cernmx.cern.ch', port)
        self._mail_server_object.ehlo()
        self._mail_server_object.starttls()
        self._mail_sender_addr = sender_addr

    def close_mail_service(self):
        self._mail_server_object.quit()

    def add_mattermost_channel(self, channel, webhook, id=None):
        if id == None : id = channel
        self._message_service_dict[id] = {'type' : 'mattermost_channel', 'channel' : channel , 'webhook' : webhook} 

    def add_mail_address(self, mail_address, id=None):
        if id == None : id = mail_address
        self._message_service_dict[id] = {'type' : 'e-mail', 'address' : mail_address}

    def add_phone_number(self, phone_number, id=None):
        if id == None : id = phone_number
        self._message_service_dict[id] = {'type' : 'phone_number', 'number' : phone_number}
    
    def remove_id(self, id):
        del self._message_service_dict[id] 

    def send_message(self, message_subject, message, id=None):
        if self._message_service_dict[id]['type'] == 'mattermost_channel':
            msg = message_subject + ':' + message
            return self._send_to_mattermost(id=id, message=msg)
        elif self._message_service_dict[id]['type'] == 'e-mail':
            mail_address = self._message_service_dict[id]['address']
            return self._send_to_email(recv_address=mail_address, message_subject=message_subject, message=message)
        elif self._message_service_dict[id]['type'] == 'phone_number':
            phone_number = self._message_service_dict[id]['number']
            return self._send_to_phone(recv_phone_number=phone_number, message_subject=message_subject, message=message)
        else:
            print(f'Warning: {id} is not a valid receiver. Message is not sent.')
            return False
    
    def send_to_all(self, message_subject, message):
        sending_status = {}
        for key, value in self._message_service_dict.items():
            sending_status[key] = self.send_message(id=key, message_subject=message_subject, message=message)
        return sending_status

class Watchdog(MessagePigeon):
    def __init__(self, directory_list=None, recursive=True):
        super().__init__()
        self._path_list = []
        self._directory_size_dict = {}
        self._recursive = recursive

        for dir in directory_list:
            self._start_tracking_directory(path=dir)

    def _get_file_list(self, path):
        ret = []
        for d,_,files in os.walk(path):
            for f in files:
                ret.append(os.path.join(d,f))
            if not self._recursive: break
        return ret
    
    def _get_size(self, path):
        files = self._get_file_list(path=path,)
        return sum(os.path.getsize(f) for f in files)

    def _start_tracking_directory(self, path):
        path = os.path.abspath(path)
        self._path_list.append(path)
        self._directory_size_dict[path] = {'directory_size' : 0}
        self._directory_size_dict[path]['directory_size'] = self._get_size(path=path)
        print(f'Started tracking: {path}')
        return self._directory_size_dict[path]['directory_size']
    
    def _directory_has_increased(self, path):
        if path not in self._directory_size_dict:
            print(f'Warning: {path} is not a tracked directory')
            return None

        dir_size_new = self._get_size(path=path,)
        ret = dir_size_new > self._directory_size_dict[path]['directory_size']
        self._directory_size_dict[path]['directory_size'] = dir_size_new
        return ret
    
    def _run(self, interval=5):
        while True:
            time.sleep(interval*60)

            directory_status = []
            for path in self._path_list:
                ret = self._directory_has_increased(path=path)
                if not ret:
                    directory_status.append(path)

            if directory_status:
                msg_type = 'Warning'
                msg = f'The follwoig directory(s): {directory_status} has not increased for the last {interval} minute(s).'
                print(msg_type + ': ' + msg)
                self.send_to_all(message_subject=msg_type, message=msg)
        
    def run(self, interval=5, silent=False):
        try:
            if not silent:
                msg_type = 'Status update'
                msg = f'Starting tracking directory(s): {self._path_list}. With and interval of {interval} minute(s).'
                print(msg_type + ': ' + msg)
                self.send_to_all(message_subject=msg_type, message=msg)
            self._run(interval=interval)
        except KeyboardInterrupt:
            if not silent:
                msg_type = 'Status update'
                msg = f'Stopped tracking directory(s): {self._path_list}.'
                print(msg_type + ': ' + msg)
                self.send_to_all(message_subject=msg_type, message=msg)
        except Exception as e:
            if not silent:
                msg_type = 'Error'
                msg = f'Program terminated with exception: {e} \nWarning: Stopped tracking directory(s): {self._path_list}'
                print(msg_type + ': ' + msg)
                self.send_to_all(message_subject=msg_type, message=msg)
            raise e

if __name__=='__main__':
    parser = argparse.ArgumentParser('Monitor if the size of a directory change.')
    parser.add_argument('directory', nargs='*', help='Directory(s) to monitor for changes.')
    parser.add_argument('--not_recursive', '-nr', action='store_false', help='Monitor only given directory, and not sub directories.')
    parser.add_argument('--interval', '-i', default=5, type=float, help='Monitoring interval in minutes.')
    parser.add_argument('--mm_webhook', '-wh', help='Mattermost webhook URL')
    parser.add_argument('--mm_channel', '-ch', nargs='*', default='', help='Name of mattermost channel(s) (or @USERNAME) to notify. See: https://developers.mattermost.com/integrate/webhooks/incoming/')
    parser.add_argument('--email', '-e', nargs='*', default='', help='E-mail(s) to notify.')
    parser.add_argument('--phone_number', '-pn', nargs='*', default='', help='Phone number(s) to notify. Uses same message format as e-mail. Only works with CERN phone numbers.')
    parser.add_argument('--silent', '-s', action='store_true', help='Only send messages when there are warnings or errors.')
    parser.add_argument('--test', action='store_true', help='Test connection to Mattermost/email/phone, then exit.')
    args = parser.parse_args()

    path_list = args.directory
    watchdog = Watchdog(directory_list=path_list, recursive=args.not_recursive)

    channel_list = args.mm_channel
    for channel in channel_list:
        watchdog.add_mattermost_channel(channel=channel, webhook=args.mm_webhook)
    
    email_list = args.email
    for email in email_list:
        watchdog.add_mail_address(mail_address=email)
    
    phone_number_list = args.phone_number
    for phone_number in phone_number_list:
        watchdog.add_phone_number(phone_number=phone_number)

    if (len(email_list) +  len(phone_number_list)) > 0:
        print('Connecting to mailing service...')
        watchdog.start_mail_service(sender_addr='watchdog-noreply@cern.ch')
        print('Connected!')
    
    if args.test:
        msg_type = 'Test message'
        msg = f'Watchdog configured to track the following directory(s): {path_list}.'
        print(msg_type + ': ' + msg)
        ret = watchdog.send_to_all(message_subject=msg_type, message=msg)
        if ret:
            print('Sending test message and exiting')
        else:
            print(f'Warning: Could not reach all channels/e-mails')
        exit(0)

    watchdog.run(interval=args.interval, silent=args.silent)
