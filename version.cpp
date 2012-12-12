const char *get_qaac_version()
{
#ifdef REFALAC
    return "1.08";
#else
    return "2.08";
#endif
}
