#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

typedef struct{
    sem_t mutex;
    sem_t s_corridori_attendi_via;
    sem_t s_arbitro_via, s_arbitro_risultato;

    int bandierina_presa;
    int arbitro_pronto_al_via;
    int arbitro_pronto_risultato;

    int giocatori_bloccati, preso_1, preso_2, salvo_1, salvo_2;

} bandierine_t;

void init_bandierine(bandierine_t * b)
{
    sem_init(&b->mutex, 0, 1);
    sem_init(&b->s_corridori_attendi_via, 0, 0);
    sem_init(&b->s_arbitro_via, 0, 0);
    sem_init(&b->s_arbitro_risultato, 0, 0);

    b->bandierina_presa = b->arbitro_pronto_al_via = b->arbitro_pronto_risultato = b->giocatori_bloccati = b->salvo_1 = b->salvo_2 = b->preso_1 = b->preso_2 = 0;
}

void attendi_il_via(bandierine_t * b ,int n)
{
    sem_wait(&b->mutex);
    b->giocatori_bloccati ++;
    if(b->giocatori_bloccati == 2 && b->arbitro_pronto_al_via)
    {
        b->arbitro_pronto_al_via--; // rimetto a zero
        sem_post(&b->s_arbitro_via);
    }   
    sem_post(&b->mutex);
    sem_wait(&b->s_corridori_attendi_via); // non ho fatto la post preventiva quindi mi blocco sempre
}

void attendi_giocatori(bandierine_t * b)
{
    sem_wait(&b->mutex);
    if(b->giocatori_bloccati == 2)
    {
        sem_post(&b->s_arbitro_via); // post preventiva
    }
    else 
    {
        b->arbitro_pronto_al_via++;
    }
    sem_post(&b->mutex);
    sem_wait(&b->s_arbitro_via);
}

void via(bandierine_t *b)
{
    sem_wait(&b->mutex);
    for(int i = 0; i < 2; i++)
    {
        sem_post(&b->s_corridori_attendi_via);
    }
    b->giocatori_bloccati = 0;
    sem_post(&b->mutex);
}

int bandierina_presa(bandierine_t *b, int n)
{
    sem_wait(&b->mutex);
    if(b->bandierina_presa == 0) // nessuno dei due ha già preso la bandierina
    {
        b->bandierina_presa = n;
        sem_post(&b->mutex);
        return 1; // bandierina presa con successo
    }
    else 
    {
        sem_post(&b->mutex);
        return 0; //non sono riuscito a prendere la bandierina
    }
}

int risultato_gioco(bandierine_t *b) // non bloccante
{
    sem_wait(&b->mutex);

    if(!b->salvo_1 && !b->salvo_2 && !b->preso_1 && !b->preso_2) // nessuno è stato preso e nessuno si è salvato
    {
        b->arbitro_pronto_risultato++;
        sem_post(&b->mutex);
        sem_wait(&b->s_arbitro_risultato);
        b->arbitro_pronto_risultato--;
    }
    // quando mi ritrovo qui o ho ricevuto il token da uno dei due player oppure non sono mai entrato nell'if, in ogni caso ho in mano il mutex e posso dare il risultato

    /// CORREZIONE: mi salvo le variabili in locale così posso resettare le variabili contenute nella struct (per la prossima partita) prima di verificare chi ha vinto
    int s1 = b->salvo_1;
    int s2 = b->salvo_2;
    int p1 = b->preso_1;
    int p2 = b->preso_2;
    b->salvo_1 = 0;
    b->salvo_2 = 0;
    b->preso_1 = 0;
    b->preso_2 = 0;

    if(s1) // ha vinto il giocatore 1 dopo che ha preso la bandierina
    {
        sem_post(&b->mutex);
        return 1;
    }
    else if(s2) // ha vinto il giocatore 2 dopo che ha preso la bandierina
    {
        sem_post(&b->mutex);
        return 2;
    }
    else if(p1) // ha vinto il giocatore 2 acchiappando il giocatore 1
    {
        sem_post(&b->mutex);
        return 2;
    }
    else // p2: ha vinto il giocatore 1 acchiappando il giocatore 2
    {
        sem_post(&b->mutex);
        return 1;
    }
    /// FINE CORREZIONE
}

int sono_salvo(bandierine_t *b, int n)
{
    sem_wait(&b->mutex);
    if(n == 1) // io sono il giocatore 1, mi devo salvare
    {
        if(b->preso_1 == 0) // non mi ha ancora acchiappato, quindi mi salvo
        {   
            b->salvo_1 = 1;

            if(b->arbitro_pronto_risultato)
            {
                sem_post(&b->s_arbitro_risultato);
            }
            /// CORREZIONE: non avevo gestito il rilascio del mutex nel momento in cui non devo fare token passing
            else 
            {
                sem_post(&b->mutex);
            }
            return 1; // sono riuscito a salvarmi
        }
        else // mi ha già preso  
        {
            if(b->arbitro_pronto_risultato)
            {
                sem_post(&b->s_arbitro_risultato);
            }
            /// CORREZIONE: non avevo gestito il rilascio del mutex nel momento in cui non devo fare token passing
            else 
            {
                sem_post(&b->mutex);
            }
            return 0; // sono riuscito a salvarmi
        }
    }
    else // io sono il giocatore 2, mi devo salvare
    {
        if(b->preso_2 == 0) // non mi ha ancora acchiappato, quindi mi salvo
        {   
            b->salvo_2 = 1;

            if(b->arbitro_pronto_risultato)
            {
                sem_post(&b->s_arbitro_risultato);
            }
            /// CORREZIONE: non avevo gestito il rilascio del mutex nel momento in cui non devo fare token passing
            else 
            {
                sem_post(&b->mutex);
            }
            return 1; // sono riuscito a salvarmi
        }
        else // mi ha già preso  
        {
            if(b->arbitro_pronto_risultato)
            {
                sem_post(&b->s_arbitro_risultato);
            }
            /// CORREZIONE: non avevo gestito il rilascio del mutex nel momento in cui non devo fare token passing
            else 
            {
                sem_post(&b->mutex);
            }
            return 0; // sono riuscito a salvarmi
        }
    }
}

int ti_ho_preso(bandierine_t *b, int n) // non bloccante
{
    sem_wait(&b->mutex);

    if(n == 1) // io sono il giocatore 1 quindi devo prendere il giocatore 2
    {
        if(b->salvo_2 == 0) // il giocatore 2 non si è ancora salvato quindi lo posso prendere
        {
            b->preso_2 = 1;
            if(b->arbitro_pronto_risultato) // se l'arbitro è addormentato sul semaforo del risultato lo sveglio
            {
                sem_post(&b->s_arbitro_risultato); // non posto il mutex perchè sto facendo token passing
            }
            /// CORREZIONE: non avevo gestito il rilascio del mutex nel momento in cui non devo fare token passing
            else
            {
                sem_post(&b->mutex); // posto il mutex e ritorno
            }
            return 1; // sono riuscito a prendere il giocatore 2 con successo
        }
        else // il giocatore 2 si è già salvato
        {
            if(b->arbitro_pronto_risultato) // se l'arbitro è addormentato sul semaforo del risultato lo sveglio
            {
                sem_post(&b->s_arbitro_risultato); // non posto il mutex perchè sto facendo token passing
            }
            /// CORREZIONE: non avevo gestito il rilascio del mutex nel momento in cui non devo fare token passing
            else 
            {
                sem_post(&b->mutex); // posto il mutex e ritorno
            }
            return 0; // non sono riuscito a prendere il giocatore 2
        }
    }

    else // io sono il giocatore 2 quindi devo prendere il giocatore 1
    {
        if(b->salvo_1 == 0) // il giocatore 1 non si è ancora salvato quindi lo posso prendere
        {
            b->preso_1 = 1;
            if(b->arbitro_pronto_risultato) // se l'arbitro è addormentato sul semaforo del risultato lo sveglio
            {
                sem_post(&b->s_arbitro_risultato); // non posto il mutex perchè sto facendo token passing
            }
            /// CORREZIONE: non avevo gestito il rilascio del mutex nel momento in cui non devo fare token passing
            else
            {
                sem_post(&b->mutex); // posto il mutex e ritorno
            }
            return 1; // sono riuscito a prendere il giocatore 1 con successo
        }
        else // il giocatore 1 si è già salvato
        {
            if(b->arbitro_pronto_risultato) // se l'arbitro è addormentato sul semaforo del risultato lo sveglio
            {
                sem_post(&b->s_arbitro_risultato); // non posto il mutex perchè sto facendo token passing
            }
            /// CORREZIONE: non avevo gestito il rilascio del mutex nel momento in cui non devo fare token passing
            else 
            {
                sem_post(&b->mutex); // posto il mutex e ritorno
            }
            return 0; // non sono riuscito a prendere il giocatore 1
        }
    }
}




