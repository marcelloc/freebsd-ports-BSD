--- auth2-chall.c.orig	Wed Jun 19 02:27:55 2002
+++ auth2-chall.c	Wed Jun 26 16:59:05 2002
@@ -40,20 +40,20 @@
 
 #ifdef BSD_AUTH
 extern KbdintDevice bsdauth_device;
-#else
-#ifdef SKEY
+#elif defined(USE_PAM)
+extern KbdintDevice pam_device;
+#elif defined(SKEY)
 extern KbdintDevice skey_device;
 #endif
-#endif
 
 KbdintDevice *devices[] = {
 #ifdef BSD_AUTH
 	&bsdauth_device,
-#else
-#ifdef SKEY
+#elif defined(USE_PAM)
+	&pam_device,
+#elif defined(SKEY)
 	&skey_device,
 #endif
-#endif
 	NULL
 };
 
@@ -63,6 +63,7 @@
 	char *devices;
 	void *ctxt;
 	KbdintDevice *device;
+	u_int nreq;
 };
 
 static KbdintAuthctxt *
@@ -90,6 +91,7 @@
 	debug("kbdint_alloc: devices '%s'", kbdintctxt->devices);
 	kbdintctxt->ctxt = NULL;
 	kbdintctxt->device = NULL;
+	kbdintctxt->nreq = 0;
 
 	return kbdintctxt;
 }
@@ -209,26 +211,26 @@
 	KbdintAuthctxt *kbdintctxt;
 	char *name, *instr, **prompts;
 	int i;
-	u_int numprompts, *echo_on;
+	u_int *echo_on;
 
 	kbdintctxt = authctxt->kbdintctxt;
 	if (kbdintctxt->device->query(kbdintctxt->ctxt,
-	    &name, &instr, &numprompts, &prompts, &echo_on))
+	    &name, &instr, &kbdintctxt->nreq, &prompts, &echo_on))
 		return 0;
 
 	packet_start(SSH2_MSG_USERAUTH_INFO_REQUEST);
 	packet_put_cstring(name);
 	packet_put_cstring(instr);
 	packet_put_cstring("");		/* language not used */
-	packet_put_int(numprompts);
-	for (i = 0; i < numprompts; i++) {
+	packet_put_int(kbdintctxt->nreq);
+	for (i = 0; i < kbdintctxt->nreq; i++) {
 		packet_put_cstring(prompts[i]);
 		packet_put_char(echo_on[i]);
 	}
 	packet_send();
 	packet_write_wait();
 
-	for (i = 0; i < numprompts; i++)
+	for (i = 0; i < kbdintctxt->nreq; i++)
 		xfree(prompts[i]);
 	xfree(prompts);
 	xfree(echo_on);
@@ -256,6 +258,10 @@
 
 	authctxt->postponed = 0;	/* reset */
 	nresp = packet_get_int();
+	if (nresp != kbdintctxt->nreq)
+		fatal("input_userauth_info_response: wrong number of replies");
+	if (nresp > 100)
+		fatal("input_userauth_info_response: too many replies");
 	if (nresp > 0) {
 		response = xmalloc(nresp * sizeof(char*));
 		for (i = 0; i < nresp; i++)
