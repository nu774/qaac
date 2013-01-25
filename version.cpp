const char *get_qaac_version()
{
#ifdef REFALAC
    return "1.13";
#else
    return "2.13";
#endif
}
